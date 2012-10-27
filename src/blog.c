/***************************************************
*
* Copyright 2007 by Sean Conner.  All Rights Reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
****************************************************/

#define _GNU_SOURCE 1

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cgilib6/util.h>
#include <cgilib6/nodelist.h>

#include "conf.h"
#include "blog.h"
#include "frontend.h"
#include "backend.h"
#include "globals.h"
#include "fix.h"

/**********************************************************************/

static void	 date_to_dir		(char *,struct btm *);
static void	 date_to_filename	(char *,struct btm *,const char *);
static void	 date_to_part		(char *,struct btm *,int);
static FILE	*open_file_r		(const char *,struct btm *);
static FILE	*open_file_w		(const char *,struct btm *);
static int	 date_check		(struct btm *);
static int	 date_checkcreate	(struct btm *);
static int	 blog_cache_day		(Blog,struct btm *);

/***********************************************************************/

Blog (BlogNew)(const char *location,const char *lockfile)
{
  Blog blog;
  int  rc;
  
  assert(location != NULL);
  assert(lockfile != NULL);
  
  umask(DEFAULT_PERMS);
  rc = chdir(location);
  if (rc != 0)
    return(NULL);
  
  blog           = malloc(sizeof(struct blog));
  memset(blog,0,sizeof(struct blog));
  blog->lockfile = strdup(lockfile);
  blog->lock     = 0;
  blog->max      = 100;
  blog->idx      = 0;
  
  memset(&blog->cache,0,sizeof(struct btm));
  
  return(blog);
}

/***********************************************************************/

int (BlogLock)(Blog blog)
{
  struct flock lockdata;
  int          rc;
  
  assert(blog != NULL);
  
  blog->lock = open(blog->lockfile,O_CREAT | O_RDWR, 0666);
  if (blog->lock == -1) return(false);
  
  lockdata.l_type   = F_WRLCK;
  lockdata.l_start  = 0;
  lockdata.l_whence = SEEK_SET;
  lockdata.l_len    = 0;
  
  rc = fcntl(blog->lock,F_SETLKW,&lockdata);

  if (rc < 0)
  {
    close(blog->lock);
    return(false);
  }

  return (true);
}

/***********************************************************************/

int (BlogUnlock)(Blog blog)
{
  struct flock lockdata;
  int          rc;
  
  assert(blog       != NULL);
  assert(blog->lock >  0);
  
  lockdata.l_type   = F_UNLCK;
  lockdata.l_start  = 0;
  lockdata.l_whence = SEEK_SET;
  lockdata.l_len    = 0;
  
  rc = fcntl(blog->lock,F_SETLK,&lockdata);
  if (rc == 0)
  {
    close(blog->lock);
    blog->lock = 0;
    return(true);
  }
  else
    return(false);
}

/***********************************************************************/

int (BlogFree)(Blog blog)
{
  size_t    i;
  
  assert(blog != NULL);
  
  if (blog->lock > 0)
    BlogUnlock(blog);
  
  for (i = 0 ; i < blog->idx ; i++)
    BlogEntryFree(blog->entries[i]);
  
  free(blog->lockfile);
  free(blog);
  return(0);
}

/************************************************************************/

BlogEntry (BlogEntryNew)(Blog blog)
{
  BlogEntry pbe;

  assert(blog != NULL);
  
  pbe               = malloc(sizeof(struct blogentry));
  pbe->node.ln_Succ = NULL;
  pbe->node.ln_Pred = NULL;
  pbe->blog         = blog;
  pbe->when.year    = gd.now.year;
  pbe->when.month   = gd.now.month;
  pbe->when.day     = gd.now.day;
  pbe->when.part    = 0;
  pbe->title        = strdup("");
  pbe->class        = strdup("");
  pbe->author       = strdup("");
  pbe->status       = strdup("");
  pbe->body         = strdup("");
  
  return(pbe);
}

/***********************************************************************/

BlogEntry (BlogEntryRead)(Blog blog,struct btm *which)
{
  int part;

  assert(blog                          != NULL);
  assert(which                         != NULL);
  assert(which->part                   >  0);
  assert(btm_cmp_date(which,&gd.begin) >= 0);
  
  if (date_check(which) == false)
    return(NULL);

  while(1)
  {    
    if (btm_cmp_date(which,&blog->cache) == 0)
    {
      if (blog->idx == 0)
        return(NULL);
      
      if (which->part > blog->idx)
        return(NULL);
      
      assert(btm_cmp(&blog->entries[which->part - 1]->when,which) == 0);
      return(blog->entries[which->part - 1]);
    }
  
    part = which->part;
    which->part = 1;
    blog_cache_day(blog,which);
    which->part = part;
  }
}

/**********************************************************************/

void (BlogEntryReadBetweenU)(Blog blog,List *list,struct btm *start,struct btm *end)
{
  BlogEntry entry;
  
  assert(blog               != NULL);
  assert(start              != NULL);
  assert(end                != NULL);
  assert(btm_cmp(start,end) <= 0);
  
  while(btm_cmp(start,end) <= 0)
  {
    entry = BlogEntryRead(blog,start);
    if (entry != NULL)
    {
      ListAddTail(list,&entry->node);
      start->part++;
    }
    else
    {
      start->part = 1;
      btm_add_day(start);
    }
  }
}

/************************************************************************/

void (BlogEntryReadBetweenD)(Blog blog,List *listb,struct btm *end,struct btm *start)
{
  List  lista;
  Node *node;
  
  assert(blog               != NULL);
  assert(start              != NULL);
  assert(end                != NULL);
  assert(btm_cmp(end,start) >= 0);
  
  ListInit(&lista);
  
  /*----------------------------------------------
  ; we read in the entries from earlier to later,
  ; then just reverse the list.
  ;----------------------------------------------*/
  
  BlogEntryReadBetweenU(blog,&lista,start,end);
  
  for
  (
    node = ListRemHead(&lista);
    NodeValid(node);
    node = ListRemHead(&lista)
  )
  {
    ListAddHead(listb,node);
  }
}

/*******************************************************************/
  
void (BlogEntryReadXD)(Blog blog,List *list,struct btm *start,size_t num)
{
  BlogEntry entry;
  
  assert(blog  != NULL);
  assert(start != NULL);
  assert(num   >  0);
  
  while(num)
  {
    entry = BlogEntryRead(blog,start);
    if (entry != NULL)
    {
      ListAddTail(list,&entry->node);
      num--;
    }
    start->part--;
    if (start->part == 0)
    {
      start->part = 1;
      do 
      {
        if (btm_cmp_date(start,&gd.begin) < 0)
          return;
            
        btm_sub_day(start);
        entry = BlogEntryRead(blog,start);
      } while(entry == NULL);
      start->part = blog->idx;
    }
  }
}

/*******************************************************************/

void (BlogEntryReadXU)(Blog blog,List *list,struct btm *start,size_t num)
{
  BlogEntry entry;
  
  assert(blog  != NULL);
  assert(start != NULL);
  assert(num   >  0);
  
  while((num) && (btm_cmp_date(start,&gd.now) <= 0))
  {
    entry = BlogEntryRead(blog,start);
    if (entry != NULL)
    {
      ListAddTail(list,&entry->node);
      num--;
      start->part++;
    }
    else
    {
      start->part = 1;
      btm_add_day(start);
    }
  }
}

/**************************************************************************/

int (BlogEntryWrite)(BlogEntry entry)
{
  Blog   blog;
  char   buffer[FILENAME_MAX];
  FILE  *stitles;
  FILE  *sclass;
  FILE  *sauthors;
  FILE  *status;
  FILE  *out;
  int    rc;
  size_t i;
  
  assert(entry != NULL);
  
  /*-----------------------------------------------
  ; cache the day the entry is to be added to.  We
  ; need this information when we add the entry
  ; (since the anciliary files need to be recreated).
  ;-------------------------------------------------*/
  
  blog = entry->blog;
  rc   = blog_cache_day(blog,&entry->when);
  if (rc != 0)
    return(rc);
    
  /*---------------------------------------------
  ; if when.part is 0, then this is a new entry
  ; to be added.  Otherwise, add to the apropriate
  ; spot in the day.
  ;---------------------------------------------*/
  
  if (entry->when.part == 0)
  {
    blog->entries[blog->idx++] = entry;
    entry->when.part           = blog->idx;
  }
  else
    blog->entries[entry->when.part - 1] = entry;
  
  /*---------------------------------------------
  ; now update all the information.
  ;--------------------------------------------*/
  
  rc = date_checkcreate(&entry->when);
  if (rc != 0)
    return(rc);
  
  stitles = open_file_w("titles",&blog->cache);
  if (stitles == NULL)
    return ENOENT;
  
  sclass = open_file_w("class",&blog->cache);
  if (sclass == NULL)
  {
    fclose(stitles);
    return ENOENT;
  }
  
  sauthors = open_file_w("authors",&blog->cache);
  if (sauthors == NULL)
  {
    fclose(sclass);
    fclose(stitles);
    return ENOENT;
  }
  
  status = open_file_w("status",&blog->cache);
  if (status == NULL)
  {
    fclose(sauthors);
    fclose(sclass);
    fclose(stitles);
    return ENOENT;
  }
  
  for (i = 0 ; i < blog->idx ; i++)
  {
    fprintf(stitles, "%s\n",blog->entries[i]->title);
    fprintf(sclass,  "%s\n",blog->entries[i]->class);
    fprintf(sauthors,"%s\n",blog->entries[i]->author);
    fprintf(status,  "%s\n",blog->entries[i]->status);
  }
      
  date_to_part(buffer,&entry->when,entry->when.part);
  out = fopen(buffer,"w");
  fputs(entry->body,out);
  
  fclose(out);
  fclose(status);
  fclose(sauthors);
  fclose(sclass);
  fclose(stitles);
  
  return(0);
}

/***********************************************************************/

int (BlogEntryFree)(BlogEntry entry)
{
  assert(entry != NULL);
  
  free(entry->body);
  free(entry->status);
  free(entry->author);
  free(entry->class);
  free(entry->title);
  free(entry);
  return(0);
}

/***********************************************************************/

static void date_to_dir(char *tname,struct btm *date)
{
  assert(tname != NULL);
  assert(date  != NULL);
  
  sprintf(tname,"%04d/%02d/%02d",date->year,date->month,date->day);
}

/***********************************************************************/

static void date_to_filename(char *tname,struct btm *date,const char *file)
{
  assert(tname != NULL);
  assert(date  != NULL);
  assert(file  != NULL);
  
  sprintf(tname,"%04d/%02d/%02d/%s",date->year,date->month,date->day,file);
}

/**********************************************************************/

static void date_to_part(char *tname,struct btm *date,int p)
{
  assert(tname != NULL);
  assert(date  != NULL);
  assert(p     >  0);
  
  sprintf(tname,"%04d/%02d/%02d/%d",date->year,date->month,date->day,p);
}

/*********************************************************************/

static FILE *open_file_r(const char *name,struct btm *date)
{
  FILE *in;
  char  buffer[FILENAME_MAX];
  
  assert(name != NULL);
  assert(date != NULL);
  
  date_to_filename(buffer,date,name);
  in = fopen(buffer,"r");
  if (in == NULL)
    in = fopen("/dev/null","r");
  
  /*--------------------------------------------------
  ; because the code was written using a different IO
  ; model (that is, if there's no data in the file to
  ; begin with, we automatically end up in EOF mode,
  ; let's trigger EOF.  Sigh. (see comment below in 
  ; blog_cache_day() for some more commentary on this
  ; sad state of affairs.
  ;-------------------------------------------------*/
  
  ungetc(fgetc(in),in);
  return(in);
}

/**********************************************************************/

static FILE *open_file_w(const char *name,struct btm *date)
{
  FILE *out;
  char  buffer[FILENAME_MAX];
  
  assert(name != NULL);
  assert(date != NULL);
  
  date_to_filename(buffer,date,name);
  out = fopen(buffer,"w");
  return(out);
}

/*********************************************************************/

static int date_check(struct btm *date)
{
  int rc;
  char tname[FILENAME_MAX];
  struct stat status;
  
  date_to_dir(tname,date);
  rc = stat(tname,&status);
  return(rc == 0);
}

/************************************************************************/

static int date_checkcreate(struct btm *date)
{
  int         rc;
  char        tname[FILENAME_MAX];
  struct stat status;
  
  assert(date != NULL);
  
  sprintf(tname,"%04d",date->year);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT)
      return(errno);
    rc = mkdir(tname,0777);
    if (rc != 0)
      return(errno);
  }
  
  sprintf(tname,"%04d/%02d",date->year,date->month);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT)
      return(errno);
    rc = mkdir(tname,0777);
    if (rc != 0)
      return(errno);
  }
  
  sprintf(tname,"%04d/%02d/%02d",date->year,date->month,date->day);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT)
      return(errno);
    rc = mkdir(tname,0777);
    if (rc != 0)
      return(errno);
  }
  
  return(0);
}

/********************************************************************/

static int blog_cache_day(Blog blog,struct btm *date)
{
  FILE      *stitles;
  FILE      *sclass;
  FILE      *sauthors;
  FILE      *status;
  BlogEntry  entry;
  size_t     i;
  size_t     size;

  assert(blog != NULL);
  assert(date != NULL);
  
  /*---------------------------------------------
  ; trivial check---if we already have the data
  ; for this date, then don't bother re-reading
  ; anything.
  ;--------------------------------------------*/
  
  if (btm_cmp_date(&blog->cache,date) == 0)
    return(0);
  
  /*------------------------------------------
  ; free any nodes not in use, so we don't
  ; leak memory.
  ;
  ; Note, there's some suble bug somewhere that's
  ; causing the nodes to get bogus node pointers.
  ;-----------------------------------------------*/
  
  for (i = 0 ; i < blog->idx ; i++)
  {
    /* if (!NodeValid(&blog->entries[i]->node)) */
    if (
         (blog->entries[i]->node.ln_Succ == NULL)
	 && (blog->entries[i]->node.ln_Pred == NULL)
       )
    {
      BlogEntryFree(blog->entries[i]);
    }
  } 
  
  /*--------------------------------------------
  ; read in the day's entries 
  ;-------------------------------------------*/
  
  blog->idx = 0;
  stitles   = open_file_r("titles", date);
  sclass    = open_file_r("class",  date);
  sauthors  = open_file_r("authors",date);
  status    = open_file_r("status", date);
  
  while(!feof(stitles) || !feof(sclass) || !feof(sauthors) || !feof(status))
  {
    char  pname[FILENAME_MAX];
    char *p;
    
    entry               = malloc(sizeof(struct blogentry));
    entry->node.ln_Succ = NULL;
    entry->node.ln_Pred = NULL;
    entry->when.year    = date->year;
    entry->when.month   = date->month;
    entry->when.day     = date->day;
    entry->title        = NULL;
    entry->class        = NULL;
    entry->author       = NULL;
    entry->status       = NULL;
    entry->body         = NULL;
    
    size = 0;
    if (!feof(stitles))
    {
      if (getline(&entry->title,&size,stitles) == -1)
      {
        free(entry->title);
        entry->title = strdup("");
	size = 0;
      }
    }
    else
      entry->title = strdup("");
    
    p = memchr(entry->title,'\n',size); if (p) *p = '\0';
    
    size = 0;
    if (!feof(sclass))
    {
      if (getline(&entry->class,&size,sclass) == -1)
      {
        free(entry->class);
        entry->class = strdup("");
	size = 0;
      }
    }
    else
      entry->class = strdup("");
    
    p = memchr(entry->class,'\n',size); if (p) *p = '\0';
    
    size = 0;
    if (!feof(sauthors))
    {
      if (getline(&entry->author,&size,sauthors) == -1)
      {
        free(entry->author);
        entry->author = strdup("");
	size = 0;
      }
    }
    else
      entry->author = strdup("");
    
    p = memchr(entry->author,'\n',size); if (p) *p = '\0';
    
    size = 0;
    if (!feof(status))
    {
      if (getline(&entry->status,&size,status) == -1)
      {
        free(entry->status);
        entry->status = strdup("");
        size = 0;
      }
    }
    else
      entry->status = strdup("");
    
    p = memchr(entry->status,'\n',size); if (p) *p = '\0';
    
    date_to_part(pname,date,blog->idx + 1);
    
    {
      FILE        *sinbody;
      struct stat  status;
      int          rc;
      
      rc = stat(pname,&status);
      if (rc == 0)
        entry->timestamp = status.st_mtime;
      else
        entry->timestamp = gd.tst;
      
      sinbody = fopen(pname,"r");
      if (sinbody == NULL)
        entry->body = strdup("");
      else
      {
        entry->body = malloc(status.st_size + 1);
        fread(entry->body,1,status.st_size,sinbody);
        fclose(sinbody);
        entry->body[status.st_size] = '\0';
      }
    }
    
    blog->entries[blog->idx++] = entry;
    entry->when.part           = blog->idx;
    
    /*-------------------------------------------------
    ; Sigh.  SCL (Standard C Library) doesn't set end-of-file
    ; until you actually attempt to READ, even if it
    ; is indeed at the end of the file.  So this lovely
    ; little bit of code reads and unreads some data
    ; from each file to trigger the eof condition.
    ;--------------------------------------------------*/
    
    ungetc(fgetc(stitles),stitles);
    ungetc(fgetc(sclass),sclass);
    ungetc(fgetc(sauthors),sauthors);
    ungetc(fgetc(status),status);
  }
  
  blog->cache = *date;
  fclose(status);
  fclose(sauthors);
  fclose(sclass);
  fclose(stitles);
  return(0);
}

/************************************************************************/

