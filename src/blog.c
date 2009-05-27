/***************************************************
*
* Copyright 2001 by Sean Conner.  All Rights Reserved.
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

static char	*filename_from_date	(char *,const char *,size_t,struct tm *);
static int	 date_check		(struct tm *);
static int	 date_checkcreate	(struct tm *);
static Stream	 efile_open_r		(const char *,struct tm *);
static Stream	 efile_open_sw		(const char *,const char *);
static Stream	 efile_open_iw		(int,const char *);

/***********************************************************************/

int (BlogInit)(void)
{
  int rc;
  
  umask(DEFAULT_PERMS);
  rc = chdir(c_basedir);
  if (rc != 0)
    return(ERR_ERR);
  
  return(ERR_OKAY);
}

/***********************************************************************/

int (BlogLock)(const char *lf)
{
  struct flock lockdata;
  int          fh;
  int          rc;
  
  ddt(lf != NULL);
  
  fh = open(lf,O_CREAT | O_RDWR, 0666);
  if (fh == -1) return(FALSE);
  
  lockdata.l_type   = F_WRLCK;
  lockdata.l_start  = 0;
  lockdata.l_whence = SEEK_SET;
  lockdata.l_len    = 0;
  
  rc = fcntl(fh,F_SETLKW,&lockdata);

  if (rc < 0)
  {
    close(fh);
    return(-1);
  }

  return (fh);
}

/***********************************************************************/

int (BlogUnlock)(int lock)
{
  struct flock lockdata;
  int          rc;
  
  ddt(lock > 0);
  
  lockdata.l_type   = F_UNLCK;
  lockdata.l_start  = 0;
  lockdata.l_whence = SEEK_SET;
  lockdata.l_len    = 0;
  
  rc = fcntl(lock,F_SETLK,&lockdata);
  if (rc == 0)
  {
    close(lock);
    return(TRUE);
  }
  else
    return(FALSE);
}

/***********************************************************************/

static char *filename_from_date(char *out,const char *name,size_t size,struct tm *date)
{
  ddt(out  != NULL);
  ddt(name != NULL);
  ddt(size >  0);
  ddt(date != NULL);
  
  sprintf(
  	out,
  	"%04d/%02d/%02d/%s",
  	date->tm_year + 1900,
  	date->tm_mon  + 1,
  	date->tm_mday,
  	name
  );
  return(out);
}

/***********************************************************************/

static int date_check(struct tm *date)
{
  int         rc;
  char        tname[FILENAME_LEN];
  struct stat status;
  
  strftime(tname,FILENAME_LEN,"%Y/%m/%d",date);
  rc = stat(tname,&status);
  return(rc);
}

/*********************************************************************/

static int date_checkcreate(struct tm *date)
{
  int         rc;
  char        tname[FILENAME_LEN];
  struct stat status;
  
  ddt(date != NULL);
  
  strftime(tname,FILENAME_LEN,"%Y",date);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT) 
      return(ERR_ERR);
    rc = mkdir(tname,0777);
    if (rc != 0)
      return(ERR_ERR);
  }
  
  strftime(tname,FILENAME_LEN,"%Y/%m",date);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT)
      return(ERR_ERR);
    rc = mkdir(tname,0777);
    if (rc != 0)
      return(ERR_ERR);
  }
  
  strftime(tname,FILENAME_LEN,"%Y/%m/%d",date);
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

/************************************************************************/

static Stream efile_open_r(const char *name,struct tm *date)
{
  Stream in;
  char   buffer[FILENAME_MAX];
 
  ddt(name != NULL);
  ddt(date != NULL);
 
  filename_from_date(buffer,name,FILENAME_MAX,date);
  in = FileStreamRead(buffer);
  if (in == NULL)
    in = StreamNewRead();
  return(in);
}

/************************************************************************/

static Stream efile_open_sw(const char *name,const char *date)
{
  char buffer[FILENAME_MAX];
  
  ddt(name != NULL);
  ddt(date != NULL);
  
  sprintf(buffer,"%s/%s",date,name);
  return(FileStreamWrite(buffer,FILE_CREATE));
}

/*************************************************************************/

static Stream efile_open_iw(int num,const char *date)
{
  char buffer[FILENAME_MAX];
  
  ddt(date != NULL);
  sprintf(buffer,"%s/%d",date,num);
  return(FileStreamWrite(buffer,FILE_CREATE));
}

/**************************************************************************/

int (BlogDayRead)(BlogDay *pdat,struct tm *pdate)
{
  char       date[FILENAME_LEN];
  BlogDay    dat;
  BlogEntry  entry;
  Stream     stitles;
  Stream     sclass;
  Stream     sauthors;
  char      *title;
  char      *class;
  char      *author;

  ddt(pdat  != NULL);
  ddt(pdate != NULL);
  
  strftime(date,FILENAME_LEN,"%Y/%m/%d",pdate);

  dat           = MemAlloc(sizeof(struct blogday));
  dat->number   = 0;
  dat->curnum   = 0;
  dat->stentry  = 0;
  dat->endentry = -1;
  dat->date     = dup_string(date);
  dat->tm_date  = *pdate;
  *pdat         = dat;
  
  if (date_check(pdate) != ERR_OKAY)
  {
    /*-----------------------------------------------------------
    ; This is probably the first entry for the day in question.
    ; We won't return an error just yet, but return an empty
    ; LogDay.  Only later, when we attempt to actually create the
    ; entry will an error be returned.
    ;------------------------------------------------------------*/
    return(ERR_OKAY);
  }

  stitles  = efile_open_r("titles",pdate);
  sclass   = efile_open_r("class",pdate);
  sauthors = efile_open_r("authors",pdate);
   
#if 0
  if ((stitles == NULL) && (sclass == NULL) && (sauthors == NULL))
  {
    /*------------------------------------------------
    ; same condition as above, really
    ;------------------------------------------------*/

    return(ERR_OKAY);
  }
#endif

  /*-------------------------------------------------
  ; Now, we have either both files open, or only one
  ; of the two.  This isn't that critical of an error,
  ; but for the sake of mistakes, we'll press on.
  ;--------------------------------------------------*/
  
  while(!StreamEOF(stitles) || !StreamEOF(sclass) || !StreamEOF(sauthors))
  {
    if (!StreamEOF(stitles))
      title = LineSRead(stitles);
    else
      title = dup_string("");
      
    if (!StreamEOF(sclass))
      class = LineSRead(sclass);
    else
      class = dup_string("");
    
    if (!StreamEOF(sauthors))
      author = LineSRead(sauthors);
    else
      author = dup_string("");
    
    BlogEntryNew(&entry,title,class,author,NULL,0);
    BlogDayEntryAdd(dat,entry);
  }
  
  StreamFree(sauthors);
  StreamFree(sclass);
  StreamFree(stitles);
  return(ERR_OKAY);
}

/*********************************************************************/    

int (BlogDayWrite)(BlogDay day)
{
  Stream    stitles;
  Stream    sclass;
  Stream    sauthors;
  BlogEntry entry;
  int       i;
  int       rc;
  
  if (date_checkcreate(&day->tm_date))
    return(ERR_ERR);
  
  stitles = efile_open_sw("titles",day->date);
  if (stitles == NULL)
    return(ERR_ERR);
    
  sclass  = efile_open_sw("class",day->date);
  if (sclass == NULL)
  {
    StreamFree(stitles);
    return(ERR_ERR);
  }
  
  sauthors = efile_open_sw("authors",day->date);
  if (sauthors == NULL)
  {
    StreamFree(sclass);
    StreamFree(stitles);
    return(ERR_ERR);
  }
  
  for (i = 0 ; i < day->number ; i++)
  {
    entry = day->entries[i];
    if (entry->body)
    {
      rc = BlogEntryWrite(entry);
      if (rc != ERR_OKAY)
      {
        StreamFree(sauthors);
        StreamFree(sclass);
        StreamFree(stitles);
        return(ERR_ERR);
      }
    }
    LineSFormat(stitles, "$","%a\n",entry->title);
    LineSFormat(sclass,  "$","%a\n",entry->class);
    LineSFormat(sauthors,"$","%a\n",entry->author);
  }

  StreamFree(sauthors);
  StreamFree(sclass);
  StreamFree(stitles);  
  return(ERR_OKAY);
}

/*********************************************************************/

int (BlogDayFree)(BlogDay *pdb)
{
  BlogDay day;
  int     i;
  
  ddt(pdb != NULL);
  
  day = *pdb;
  
  for (i = 0 ; i < day->number ; i++)
    BlogEntryFree(&day->entries[i]);
  
  MemFree(day->date);
  MemFree(day);
  *pdb = NULL;
  return(ERR_OKAY);
}

/**********************************************************************/

int (BlogEntryNew)(BlogEntry *pentry,const char *title,const char *class,const char *author,void *body,size_t bsize)
{
  BlogEntry lentry;

  ddt(pentry != NULL);
  ddt(title  != NULL);
  ddt(class  != NULL);
  ddt(author != NULL);
  
  lentry         = MemAlloc(sizeof(struct blogentry));
  lentry->date   = NULL;
  lentry->number = 0;
  lentry->title  = title;
  lentry->class  = class;
  lentry->author = author;
  lentry->body   = body;
  lentry->bsize  = bsize;
  *pentry        = lentry;
  return(ERR_OKAY);
}

/*********************************************************************/

int (BlogDayEntryAdd)(BlogDay day,BlogEntry entry)
{
  ddt(day   != NULL);
  ddt(entry != NULL);
  
  entry->date   = dup_string(day->date);
  entry->number = day->number + 1;
  day->entries[day->number++] = entry;
  day->endentry++;
  return(ERR_OKAY);
}

/*****************************************************************/

int (BlogEntryRead)(BlogEntry entry)
{
  struct stat status;
  int         rc;
  Stream      in;
  char        fname[FILENAME_LEN];
  size_t      size;
  
  sprintf(fname,"%s/%d",entry->date,entry->number);
  rc = stat(fname,&status);
  if (rc != 0)
    return(ERR_ERR);
  
  entry->timestamp = status.st_mtime;
  entry->body      = MemAlloc(status.st_size + 1);
  memset(entry->body,0,status.st_size + 1);
  in = FileStreamRead(fname);
  if (in == NULL)
  {
    MemFree(entry->body);
    return(ERR_ERR);
  }

  for ( size = 0 ; size < status.st_size ; size++)
    ((char *)entry->body)[size] = StreamRead(in);
  
  ((char *)entry->body)[size] = '\0';
  entry->bsize = size;
  StreamFree(in);
  return(ERR_OKAY);
}

/********************************************************************/

int (BlogEntryWrite)(BlogEntry entry)
{
  Stream out;
  size_t size;
  
  ddt(entry != NULL);
  
  out = efile_open_iw(entry->number,entry->date);
  if (out == NULL)
    return(ERR_ERR);
  
  for (size = 0 ; size < entry->bsize ; size++)
    StreamWrite(out,((char *)entry->body)[size]);

  StreamFree(out);    
  return(ERR_OKAY);
}

/************************************************************************/

int (BlogEntryFlush)(BlogEntry entry)
{
  ddt(entry != NULL);

  return(ERR_OKAY);
}


/************************************************************************/

int (BlogEntryFree)(BlogEntry *pentry)
{
  BlogEntry entry;
  
  ddt(pentry  != NULL);
  ddt(*pentry != NULL);

  entry = *pentry;
  
  if (entry->date)   MemFree(entry->date);
  if (entry->title)  MemFree((char *)entry->title);
  if (entry->class)  MemFree((char *)entry->class);
  if (entry->author) MemFree((char *)entry->author);
  if (entry->body)   MemFree(entry->body);
  MemFree(entry);
  *pentry = NULL;
  return(ERR_OKAY);
}

