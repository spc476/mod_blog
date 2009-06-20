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

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cgilib/ddt.h>
#include <cgilib/errors.h>
#include <cgilib/memory.h>
#include <cgilib/util.h>

#include "conf.h"
#include "blog.h"
#include "globals.h"

/**********************************************************************/

static void	 date_to_dir		(char *,struct btm *);
static void	 date_to_filename	(char *,struct btm *,const char *);
static void	 date_to_part		(char *,struct btm *,int);
static Stream	 open_file_r		(const char *,struct btm *);
static Stream	 open_file_w		(const char *,struct btm *);
static int	 date_check		(struct btm *);
static int	 date_checkcreate	(struct btm *);
static int	 blog_cache_day		(Blog,struct btm *);

/***********************************************************************/

Blog (BlogNew)(char *location,char *lockfile)
{
  Blog blog;
  int  rc;
  
  ddt(location != NULL);
  ddt(lockfile != NULL);
  
  umask(DEFAULT_PERMS);
  rc = chdir(location);
  if (rc != 0)
    return(NULL);
  
  blog           = MemAlloc(sizeof(struct blog));
  memset(blog,0,sizeof(struct blog));
  blog->lockfile = dup_string(lockfile);
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
  
  ddt(blog != NULL);
  
  blog->lock = open(blog->lockfile,O_CREAT | O_RDWR, 0666);
  if (blog->lock == -1) return(FALSE);
  
  lockdata.l_type   = F_WRLCK;
  lockdata.l_start  = 0;
  lockdata.l_whence = SEEK_SET;
  lockdata.l_len    = 0;
  
  rc = fcntl(blog->lock,F_SETLKW,&lockdata);

  if (rc < 0)
  {
    close(blog->lock);
    return(FALSE);
  }

  return (TRUE);
}

/***********************************************************************/

int (BlogUnlock)(Blog blog)
{
  struct flock lockdata;
  int          rc;
  
  ddt(blog       != NULL);
  ddt(blog->lock >  0);
  
  lockdata.l_type   = F_UNLCK;
  lockdata.l_start  = 0;
  lockdata.l_whence = SEEK_SET;
  lockdata.l_len    = 0;
  
  rc = fcntl(blog->lock,F_SETLK,&lockdata);
  if (rc == 0)
  {
    close(blog->lock);
    blog->lock = 0;
    return(TRUE);
  }
  else
    return(FALSE);
}

/***********************************************************************/

int (BlogFree)(Blog blog)
{
  size_t    i;
  
  ddt(blog != NULL);
  
  if (blog->lock > 0)
    BlogUnlock(blog);
  
  for (i = 0 ; i < blog->idx ; i++)
    BlogEntryFree(blog->entries[i]);
  
  MemFree(blog->lockfile);
  MemFree(blog);
  return(ERR_OKAY);
}

/************************************************************************/

BlogEntry (BlogEntryNew)(Blog blog)
{
  BlogEntry pbe;

  ddt(blog != NULL);
  
  pbe               = MemAlloc(sizeof(struct blogentry));
  pbe->node.ln_Succ = NULL;
  pbe->node.ln_Pred = NULL;
  pbe->blog         = blog;
  pbe->when.year    = gd.now.year;
  pbe->when.month   = gd.now.month;
  pbe->when.day     = gd.now.day;
  pbe->when.part    = 0;
  pbe->title        = dup_string("");
  pbe->class        = dup_string("");
  pbe->author       = dup_string("");
  pbe->body         = dup_string("");
  
  return(pbe);
}

/***********************************************************************/

BlogEntry (BlogEntryRead)(Blog blog,struct btm *which)
{
  int part;

  ddt(blog                          != NULL);
  ddt(which                         != NULL);
  ddt(which->part                   >  0);
  ddt(btm_cmp_date(which,&gd.begin) >= 0);
  
  if (date_check(which) == FALSE)
    return(NULL);

  while(1)
  {    
    if (btm_cmp_date(which,&blog->cache) == 0)
    {
      if (blog->idx == 0)
        return(NULL);
      
      if (which->part > blog->idx)
        return(NULL);
      
      ddt(btm_cmp(&blog->entries[which->part - 1]->when,which) == 0);
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
  
  ddt(blog               != NULL);
  ddt(start              != NULL);
  ddt(end                != NULL);
  ddt(btm_cmp(start,end) <= 0);
  
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
  
  ddt(blog               != NULL);
  ddt(start              != NULL);
  ddt(end                != NULL);
  ddt(btm_cmp(end,start) >= 0);
  
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
  
  ddt(blog  != NULL);
  ddt(start != NULL);
  ddt(num   >  0);
  
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
  
  ddt(blog  != NULL);
  ddt(start != NULL);
  ddt(num   >  0);
  
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
  Stream stitles;
  Stream sclass;
  Stream sauthors;
  Stream out;
  int    rc;
  size_t i;
  
  ddt(entry != NULL);
  
  /*-----------------------------------------------
  ; cache the day the entry is to be added to.  We
  ; need this information when we add the entry
  ; (since the anciliary files need to be recreated).
  ;-------------------------------------------------*/
  
  blog = entry->blog;
  rc   = blog_cache_day(blog,&entry->when);
  if (rc != ERR_OKAY)
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
  if (rc != ERR_OKAY)
    return(rc);
  
  stitles = open_file_w("titles",&blog->cache);
  if (stitles == NULL)
    return(ERR_ERR);
  
  sclass = open_file_w("class",&blog->cache);
  if (sclass == NULL)
  {
    StreamFree(stitles);
    return(ERR_ERR);
  }
  
  sauthors = open_file_w("authors",&blog->cache);
  if (sauthors == NULL)
  {
    StreamFree(sclass);
    StreamFree(stitles);
    return(ERR_ERR);
  }
  
  for (i = 0 ; i < blog->idx ; i++)
  {
    LineSFormat(stitles, "$","%a\n",blog->entries[i]->title);
    LineSFormat(sclass,  "$","%a\n",blog->entries[i]->class);
    LineSFormat(sauthors,"$","%a\n",blog->entries[i]->author);
  }
      
  date_to_part(buffer,&entry->when,entry->when.part);
  out = FileStreamWrite(buffer,FILE_RECREATE);
  LineS(out,entry->body);
  
  StreamFree(out);
  StreamFree(sauthors);
  StreamFree(sclass);
  StreamFree(stitles);
  
  return(ERR_OKAY);
}

/***********************************************************************/

int (BlogEntryFree)(BlogEntry entry)
{
  ddt(entry != NULL);
  
  MemFree(entry->body);
  MemFree(entry->author);
  MemFree(entry->class);
  MemFree(entry->title);
  MemFree(entry);
  return(ERR_OKAY);
}

/***********************************************************************/

static void date_to_dir(char *tname,struct btm *date)
{
  ddt(tname != NULL);
  ddt(date  != NULL);
  
  sprintf(tname,"%04d/%02d/%02d",date->year,date->month,date->day);
}

/***********************************************************************/

static void date_to_filename(char *tname,struct btm *date,const char *file)
{
  ddt(tname != NULL);
  ddt(date  != NULL);
  ddt(file  != NULL);
  
  sprintf(tname,"%04d/%02d/%02d/%s",date->year,date->month,date->day,file);
}

/**********************************************************************/

static void date_to_part(char *tname,struct btm *date,int p)
{
  ddt(tname != NULL);
  ddt(date  != NULL);
  ddt(p     >  0);
  
  sprintf(tname,"%04d/%02d/%02d/%d",date->year,date->month,date->day,p);
}

/*********************************************************************/

static Stream open_file_r(const char *name,struct btm *date)
{
  Stream in;
  char   buffer[FILENAME_MAX];
  
  ddt(name != NULL);
  ddt(date != NULL);
  
  date_to_filename(buffer,date,name);
  in = FileStreamRead(buffer);
  if (in == NULL)
    in = StreamNewRead();
  return(in);
}

/**********************************************************************/

static Stream open_file_w(const char *name,struct btm *date)
{
  Stream out;
  char   buffer[FILENAME_MAX];
  
  ddt(name != NULL);
  ddt(date != NULL);
  
  date_to_filename(buffer,date,name);
  out = FileStreamWrite(buffer,FILE_RECREATE);
  return(out);
}

/*********************************************************************/

static int date_check(struct btm *date)
{
  int rc;
  char tname[FILENAME_LEN];
  struct stat status;
  
  date_to_dir(tname,date);
  rc = stat(tname,&status);
  return(rc == 0);
}

/************************************************************************/

static int date_checkcreate(struct btm *date)
{
  int         rc;
  char        tname[FILENAME_LEN];
  struct stat status;
  
  ddt(date != NULL);
  
  sprintf(tname,"%04d",date->year);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT)
      return(ERR_ERR);
    rc = mkdir(tname,0777);
    if (rc != 0)
      return(ERR_ERR);
  }
  
  sprintf(tname,"%04d/%02d",date->year,date->month);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT)
      return(ERR_ERR);
    rc = mkdir(tname,0777);
    if (rc != 0)
      return(ERR_ERR);
  }
  
  sprintf(tname,"%04d/%02d/%02d",date->year,date->month,date->day);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT)
      return(ERR_ERR);
    rc = mkdir(tname,0777);
    if (rc != 0)
      return(ERR_ERR);
  }
  
  return(ERR_OKAY);
}

/********************************************************************/

static int blog_cache_day(Blog blog,struct btm *date)
{
  Stream    stitles;
  Stream    sclass;
  Stream    sauthors;
  BlogEntry entry;
  size_t    i;

  ddt(blog != NULL);
  ddt(date != NULL);
  
  /*---------------------------------------------
  ; trivial check---if we already have the data
  ; for this date, then don't bother re-reading
  ; anything.
  ;--------------------------------------------*/
  
  if (btm_cmp_date(&blog->cache,date) == 0)
    return(ERR_OKAY);
  
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
  
  while(!StreamEOF(stitles) || !StreamEOF(sclass) || !StreamEOF(sauthors))
  {
    char   pname[FILENAME_LEN];

    entry               = MemAlloc(sizeof(struct blogentry));
    entry->node.ln_Succ = NULL;
    entry->node.ln_Pred = NULL;
    entry->when.year    = date->year;
    entry->when.month   = date->month;
    entry->when.day     = date->day;
    
    if (!StreamEOF(stitles))
      entry->title = LineSRead(stitles);
    else
      entry->title = dup_string("");
    
    if (!StreamEOF(sclass))
      entry->class = LineSRead(sclass);
    else
      entry->class = dup_string("");
    
    if (!StreamEOF(sauthors))
      entry->author = LineSRead(sauthors);
    else
      entry->author = dup_string("");
      
    date_to_part(pname,date,blog->idx + 1);
    
    {
      Stream      sinbody;
      Stream      soutbody;
      struct stat status;
      int         rc;
      
      rc = stat(pname,&status);
      if (rc == 0)
        entry->timestamp = status.st_mtime;
      else
        entry->timestamp = gd.tst;
      
      sinbody = FileStreamRead(pname);
      if (sinbody == NULL)
        entry->body = dup_string("");
      else
      {
        soutbody = StringStreamWrite();
        StreamCopy(soutbody,sinbody);
        entry->body = StringFromStream(soutbody);
        StreamFree(soutbody);
        StreamFree(sinbody);
      }
    }
    
    blog->entries[blog->idx++] = entry;
    entry->when.part           = blog->idx;
  }
  
  blog->cache = *date;
  StreamFree(sauthors);
  StreamFree(sclass);
  StreamFree(stitles);
  return(ERR_OKAY);
}

/************************************************************************/

