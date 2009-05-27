
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

#include <cgil/ddt.h>
#include <cgil/buffer.h>
#include <cgil/errors.h>
#include <cgil/memory.h>
#include <cgil/util.h>

#include "conf.h"
#include "blog.h"
#include "globals.h"

/***********************************************************************/

int (BlogInit)(void)
{
  int rc;
  
  umask(DEFAULT_PERMS);
  rc = chdir(g_basedir);
  if (rc != 0)
  {
    ErrorPush(KernErr,KERNCHDIR,errno,"$",g_basedir);
    return(ErrorPush(AppErr,BLOGINIT,errno,""));
  }
  
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
  return (rc == 0);
}

/***********************************************************************/

int (BlogUnlock)(int lock)
{
  struct flock lockdata;
  int          rc;
  
  ddt(lock > 0);
  
  lockdata.l_type = F_UNLCK;
  lockdata.l_start = 0;
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

static int date_check(struct tm *date)
{
  int         rc;
  char        tname[FILENAME_LEN];
  struct stat status;
  
  strftime(tname,FILENAME_LEN,"%Y/%m/%d",date);
  rc = stat(tname,&status);
  if (rc != 0)
    ErrorPush(KernErr,KERNSTAT,errno,"$",tname);
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
      return(ErrorPush(KernErr,KERNACCESS,errno,"$",tname));
    rc = mkdir(tname,0777);
    if (rc != 0)
      return(ErrorPush(KernErr,KERNMKDIR,errno,"$",tname));
  }
  
  strftime(tname,FILENAME_LEN,"%Y/%m",date);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT)
      return(ErrorPush(KernErr,KERNACCESS,errno,"$",tname));
    rc = mkdir(tname,0777);
    if (rc != 0)
      return(ErrorPush(KernErr,KERNMKDIR,errno,"$",tname));
  }
  
  strftime(tname,FILENAME_LEN,"%Y/%m/%d",date);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT)
      return(ErrorPush(KernErr,KERNACCESS,errno,"$",tname));
    rc = mkdir(tname,0777);
    if (rc != 0)
      return(ErrorPush(KernErr,KERNACCESS,errno,"$",tname));
  }
  return(ERR_OKAY);
}

/************************************************************************/

static FILE *efile_open(const char *name,const char *mode,struct tm *date)
{
  char  buffer[FILENAME_MAX];
  
  ddt(name != NULL);
  ddt(mode != NULL);
  ddt(date != NULL);
  
  sprintf(
           buffer,
           "%4d/%02d/%02d/%s",
           date->tm_year + 1900,
           date->tm_mon + 1,
           date->tm_mday,
           name
         );
  return(fopen(buffer,mode));
}

/************************************************************************/

static FILE *efile_open_s(const char *name,const char *mode,char *date)
{
  char  buffer[FILENAME_MAX];
  
  ddt(name != NULL);
  ddt(mode != NULL);
  ddt(date != NULL);
  
  sprintf(buffer,"%s/%s",date,name);
  return(fopen(buffer,mode));
}
/************************************************************************/

static FILE *efile_open_i(int num,const char *mode,const char *date)
{
  char  buffer[FILENAME_MAX];
  
  ddt(mode != NULL);
  ddt(date != NULL);
  
  sprintf(buffer,"%s/%d",date,num);
  return(fopen(buffer,mode));
}

/**************************************************************************/

int (BlogDayRead)(BlogDay *pdat,struct tm *pdate)
{
  char       date[FILENAME_LEN];
  BlogDay    dat;
  BlogEntry  entry;
  FILE      *fptitles;
  FILE      *fpclass;
  FILE      *fpauthors;
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
    ErrorLog();  
    ErrorClear();
    return(ERR_OKAY);
  }
    
  fptitles  = efile_open("titles","r",pdate);
  fpclass   = efile_open("class","r",pdate);
  fpauthors = efile_open("authors","r",pdate);
  
  if ((fptitles == NULL) && (fpclass == NULL) && (fpauthors == NULL))
  {
    /*------------------------------------------------
    ; same condition as above, really
    ;------------------------------------------------*/
    
    return(ERR_OKAY);
  }
  
  /*-------------------------------------------------
  ; Now, we have either both files open, or only one
  ; of the two.  This isn't that critical of an error,
  ; but for the sake of mistakes, we'll press on.
  ;--------------------------------------------------*/
  
  while((fptitles != NULL) || (fpclass != NULL) || (fpauthors != NULL))
  {
    char buffer[BUFSIZ];
    
    if (fptitles)
    {
      memset(buffer,0,sizeof(buffer));
      if (fgets(buffer,BUFSIZ,fptitles) == NULL)
      {
        fclose(fptitles);
        fptitles = NULL;
      }
      title = dup_string(remove_ctrl(buffer));
    }
    else
      title = dup_string("");
    
    if (fpclass)
    {
      memset(buffer,0,sizeof(buffer));
      if (fgets(buffer,BUFSIZ,fpclass) == NULL)
      {
        fclose(fpclass);
        fpclass = NULL;
      }
      class = dup_string(remove_ctrl(buffer));
    }
    else
      class = dup_string("");

    if (fpauthors)
    {
      memset(buffer,0,sizeof(buffer));
      if (fgets(buffer,BUFSIZ,fpauthors) == NULL)
      {
        fclose(fpauthors);
        fpauthors = NULL;
      }
      author = dup_string(remove_ctrl(buffer));
    }
    else
      author = dup_string(g_author);
      
    if ((fptitles == NULL) && (fpclass == NULL) && (fpauthors == NULL)) break;      
    BlogEntryNew(&entry,title,class,author,NULL,0);
    BlogDayEntryAdd(dat,entry);
  }

  if (fpauthors) fclose(fpauthors);  
  if (fpclass)   fclose(fpclass);
  if (fptitles)  fclose(fptitles);
  return(ERR_OKAY);
}
    
/*********************************************************************/    

int (BlogDayWrite)(BlogDay day)
{
  FILE      *fptitles;
  FILE      *fpclass;
  FILE      *fpauthors;
  BlogEntry  entry;
  int        i;
  int        rc;

  if (date_checkcreate(&day->tm_date))
    return(ErrorPush(AppErr,BLOGDAYWRITE,APPERR_OPEN,"$",day->date));
  
  fptitles = efile_open_s("titles","w",day->date);
  if (fptitles == NULL)
    return(ErrorPush(AppErr,BLOGDAYWRITE,APPERR_OPEN,"$ $",day->date,"titles"));
    
  fpclass  = efile_open_s("class","w",day->date);
  if (fpclass == NULL)
  {
    fclose(fptitles);
    return(ErrorPush(AppErr,BLOGDAYWRITE,APPERR_OPEN,"$ $",day->date,"class"));
  }
  
  fpauthors = efile_open_s("authors","w",day->date);
  if (fpauthors == NULL)
  {
    fclose(fpclass);
    fclose(fptitles);
    return(ErrorPush(AppErr,BLOGDAYWRITE,APPERR_OPEN,"$ $",day->date,"authors"));
  }
  
  for (i = 0 ; i < day->number ; i++)
  {
    entry = day->entries[i];
    if (entry->body)
    {
      rc = BlogEntryWrite(entry);
      if (rc != ERR_OKAY)
      {
        fclose(fptitles);
        fclose(fpclass);
        return(ErrorPush(AppErr,BLOGDAYWRITE,APPERR_WRITE,""));
      }
    }
    fprintf(fptitles,"%s\n",entry->title);
    fprintf(fpclass,"%s\n",entry->class);
    fprintf(fpauthors,"%s\n",entry->author);
  }
  
  fclose(fpauthors);
  fclose(fpclass);
  fclose(fptitles);
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
  
  MemFree(day->date,strlen(day->date) + 1);
  MemFree(day,sizeof(struct blogday));
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
  char        fname[FILENAME_LEN];
  Buffer      file;
  size_t      size;
  
  sprintf(fname,"%s/%d",entry->date,entry->number);
  rc = stat(fname,&status);
  if (rc != 0)
  {
    ErrorPush(KernErr,KERNSTAT,errno,"$",fname);
    return(ErrorPush(AppErr,BLOGENTRYREAD,errno,"$",fname));
  }
  
  entry->body = MemAlloc(status.st_size + 1);
  memset(entry->body,0,status.st_size + 1);
  rc = FileBuffer(&file,fname,MODE_READ);
  if (rc != ERR_OKAY)
    return(ErrorPush(AppErr,BLOGENTRYREAD,rc,"$",fname));
    
  size = status.st_size;
  rc = BufferRead(file,entry->body,&size);
  if (rc != ERR_OKAY)
    return(ErrorPush(AppErr,BLOGENTRYREAD,rc,"$",fname));
  
  ((char *)entry->body)[size] = '\0';
  entry->bsize = size;
  BufferFree(&file);
  return(ERR_OKAY);
}

/********************************************************************/

int (BlogEntryWrite)(BlogEntry entry)
{
  FILE   *fp;
  size_t  size;
  
  ddt(entry != NULL);
  
  fp = efile_open_i(entry->number,"w",entry->date);
  if (fp == NULL)
    return(ErrorPush(AppErr,BLOGENTRYWRITE,APPERR_OPEN,"$ i",entry->date,entry->number));
  
  size = fwrite(entry->body,sizeof(char),entry->bsize,fp);
  if (size != entry->bsize)
    return(ErrorPush(AppErr,BLOGENTRYWRITE,APPERR_WRITE,"$ i",entry->date,entry->number));
    
  fclose(fp);
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
  
  MemFree(entry->date,  strlen(entry->date) + 1);
  MemFree(entry->title, strlen(entry->title) + 1);
  MemFree(entry->class, strlen(entry->class) + 1);
  MemFree(entry->author,strlen(entry->author) + 1);
  MemFree(entry->body,  entry->bsize);
  MemFree(entry,        sizeof(struct blogentry));
  *pentry = NULL;
  return(ERR_OKAY);
}

