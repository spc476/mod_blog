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
#include <stdbool.h>
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
#include <syslog.h>

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
static bool	 date_check		(struct btm *);
static int	 date_checkcreate	(struct btm *);
static char     *blog_meta_entry	(const char *,struct btm *);
static size_t	 blog_meta_read		(char ***,const char *,struct btm *);
static void	 blog_meta_adjust	(char ***,size_t,size_t);
static int	 blog_meta_write	(const char *,struct btm *,char **,size_t);

/***********************************************************************/

static inline size_t max(size_t a,size_t b)
{
  if (a > b)
    return a;
  else
    return b;
}

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
  
  blog = malloc(sizeof(struct blog));
  memset(blog,0,sizeof(struct blog));
  blog->lockfile = strdup(lockfile);
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
  assert(blog != NULL);
  
  if (blog->lock > 0)
    BlogUnlock(blog);

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
  pbe->valid        = true;
  pbe->when.year    = gd.now.year;
  pbe->when.month   = gd.now.month;
  pbe->when.day     = gd.now.day;
  pbe->when.part    = 0;
  pbe->title        = NULL;
  pbe->class        = NULL;
  pbe->author       = NULL;
  pbe->status       = NULL;
  pbe->body         = NULL;
  
  return pbe;
}

/***********************************************************************/

BlogEntry (BlogEntryRead)(Blog blog,struct btm *which)
{
  BlogEntry    entry;
  char         pname[FILENAME_MAX];
  FILE        *sinbody;
  struct stat  status;
  
  assert(blog                          != NULL);
  assert(which                         != NULL);
  assert(which->part                   >  0);
  assert(btm_cmp_date(which,&gd.begin) >= 0);
  
  if (!date_check(which))
    return NULL;
  
  date_to_part(pname,which,which->part);
  if (access(pname,R_OK) != 0)
    return NULL;
  
  entry               = malloc(sizeof(struct blogentry));
  entry->node.ln_Succ = NULL;
  entry->node.ln_Pred = NULL;
  entry->valid        = true;
  entry->blog         = blog;
  entry->when.year    = which->year;
  entry->when.month   = which->month;
  entry->when.day     = which->day;
  entry->when.part    = which->part;
  entry->title        = blog_meta_entry("titles",which);
  entry->class        = blog_meta_entry("class",which);
  entry->author       = blog_meta_entry("authors",which);
  entry->status       = blog_meta_entry("status",which);

  date_to_part(pname,which,which->part);
  
  if (stat(pname,&status) == 0)
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

  return entry;  
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
  
  memset(&entry,0,sizeof(entry));
  
  while(num)
  {
    if (btm_cmp_date(start,&gd.begin) < 0)
      return;
      
    entry = BlogEntryRead(blog,start);
    if (entry != NULL)
    {
      ListAddTail(list,&entry->node);
      num--;
    }
    start->part--;
    if (start->part == 0)
    {
      start->part = 23;
      btm_sub_day(start);
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
  char   **authors;
  char   **class;
  char   **status;
  char   **titles;
  size_t   numa;
  size_t   numc;
  size_t   nums;
  size_t   numt;
  size_t   maxnum;
  char     filename[FILENAME_MAX];
  FILE    *out;
  int      rc;
  
  assert(entry != NULL);
  assert(entry->valid);
  
  rc = date_checkcreate(&entry->when);
  if (rc != 0)
    return rc;
  
  /*---------------------------------------------------------------------
  ; The meta-data for the entries are stored in separate files.  When
  ; updating an entry (or adding an entry), we need to rewrite these
  ; metafiles.  So, we read them all in, make the adjustments required and
  ; write them out.
  ;------------------------------------------------------------------------*/
  
  numa   = blog_meta_read(&authors,"authors",&entry->when);
  numc   = blog_meta_read(&class,  "class",  &entry->when);
  nums   = blog_meta_read(&status, "status", &entry->when);
  numt   = blog_meta_read(&titles, "titles", &entry->when);
  maxnum = max(numa,max(numc,max(nums,numt)));
  
  blog_meta_adjust(&authors,numa,maxnum);
  blog_meta_adjust(&class,  numc,maxnum);
  blog_meta_adjust(&status, nums,maxnum);
  blog_meta_adjust(&titles, numt,maxnum);
  
  if (entry->when.part == 0)
  {
    if (maxnum == 99)
      return ENOMEM;
    
    authors[maxnum]  = strdup(entry->author);
    class  [maxnum]  = strdup(entry->class);
    status [maxnum]  = strdup(entry->status);
    titles [maxnum]  = strdup(entry->title);
    entry->when.part = ++maxnum;
  }
  else
  {
    free(authors[entry->when.part]);
    free(class  [entry->when.part]);
    free(status [entry->when.part]);
    free(titles [entry->when.part]);
    
    authors[entry->when.part] = strdup(entry->author);
    class  [entry->when.part] = strdup(entry->class);
    status [entry->when.part] = strdup(entry->status);
    titles [entry->when.part] = strdup(entry->title);
  }
  
  blog_meta_write("authors",&entry->when,authors,maxnum);
  blog_meta_write("class"  ,&entry->when,class,  maxnum);
  blog_meta_write("status" ,&entry->when,status, maxnum);
  blog_meta_write("titles" ,&entry->when,titles, maxnum);
  
  /*-------------------------------
  ; update the actual entry body
  ;---------------------------------*/
  
  date_to_part(filename,&entry->when,entry->when.part);
  out = fopen(filename,"w");
  fputs(entry->body,out);
  fclose(out);
  
  /*-----------------
  ; clean up
  ;------------------*/
  
  for(size_t i = 0 ; i < maxnum ; i++)
  {
    free(authors[i]);
    free(class[i]);
    free(status[i]);
    free(titles[i]);
  }
  
  free(authors);
  free(class);
  free(status);
  free(titles);
  
  /*------------------------------------------------------------------------
  ; Oh, and if this is the latest entry to be added, update the .last file
  ; to reflect that.
  ;------------------------------------------------------------------------*/
  
  if (btm_cmp(&entry->when,&gd.now) > 0)
  {
    out = fopen(".last","w");
    if (out)
    {
      fprintf(
      		out,
      		"%d/%02d/%02d.%d\n",
      		entry->when.year,
      		entry->when.month,
      		entry->when.day,
      		entry->when.part
      	);
      fclose(out);
    }
  }
  
  return 0;  
}

/***********************************************************************/

int (BlogEntryFree)(BlogEntry entry)
{
  assert(entry != NULL);
  
  if (!entry->valid)
  {
    syslog(LOG_ERR,"invalid entry being freed");
    return 0;
  }
  
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

static bool date_check(struct btm *date)
{
  char tname[FILENAME_MAX];
  struct stat status;
  
  date_to_dir(tname,date);
  return stat(tname,&status) == 0;
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

static char *blog_meta_entry(const char *name,struct btm *date)
{
  FILE         *fp;
  char         *text;
  size_t        size;
  ssize_t       bytes;
  unsigned int  num;
  char         *nl;
  
  assert(name != NULL);
  assert(date != NULL);
  
  text = NULL;
  size = 0;
  num  = date->part;
  fp   = open_file_r(name,date);
  
  while(!feof(fp) && (num--))
  {
    bytes = getline(&text,&size,fp);
    if (bytes == -1)
    {
      fclose(fp);
      free(text);
      return strdup("");
    }
  };
  
  if (text)
  {
    nl = strchr(text,'\n');
    if (nl) *nl = '\0';
  }
  else
    return strdup("");
    
  return text;
}

/********************************************************************/

static size_t blog_meta_read(
	char       ***plines,
	const char   *name,
	struct btm   *date
)
{
  char   **lines;
  size_t   i;
  size_t   size;
  FILE    *fp;
  char    *nl;
  
  assert(plines != NULL);
  assert(name   != NULL);
  assert(date   != NULL);
  
  lines = malloc(100 * sizeof(char *));
  if (lines == NULL)
  {
    *plines = NULL;
    return 0;
  }
  
  fp = open_file_r(name,date);
  if (feof(fp))
  {
    fclose(fp);
    *plines = lines;
    return 0;
  }
  
  for(i = 0 ; i < 100 ; i++)
  {
    ssize_t bytes;
    
    lines[i] = NULL;
    size     = 0;
    bytes     = getline(&lines[i],&size,fp);
    if (bytes == -1)
      break;
    nl = strchr(lines[i],'\n');
    if (nl) *nl = '\0';
  }
  
  fclose(fp);
  *plines = lines;
  return i;
}

/************************************************************************/

static void blog_meta_adjust(char ***plines,size_t num,size_t maxnum)
{
  char **lines;
  
  assert(plines  != NULL);
  assert(*plines != NULL);
  assert(num     <= maxnum);
  assert(num     <= 100);
  assert(maxnum  <= 100);
  
  lines = *plines;
  
  while(num < maxnum)
    lines[num++] = strdup("");
}

/************************************************************************/

static int blog_meta_write(
	const char  *name,
	struct btm  *date,
	char       **list,
	size_t       num
)
{
  FILE   *fp;
  
  fp = open_file_w(name,date);
  if (fp == NULL)
    return errno;
  
  for (size_t i = 0 ; i < num ; i++)
    fprintf(fp,"%s\n",list[i]);
  
  fclose(fp);
  return 0;
}

/************************************************************************/
