/*********************************************************************
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
*********************************************************************/

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <syslog.h>
#include <gdbm.h>

#include <cgilib8/mail.h>
#include <cgilib8/util.h>
#include <cgilib8/chunk.h>

#include "conversion.h"
#include "backend.h"
#include "blogutil.h"

#define DB_BLOCK 1024

/*********************************************************************/

bool entry_add(Blog *blog,Request *req)
{
  BlogEntry *entry;
  bool       rc = true;
  
  assert(blog != NULL);
  assert(req  != NULL);
  
  fix_entry(req);
  
  if (blog->config.prehook != NULL)
  {
    FILE *fp;
    char  fnbody[L_tmpnam];
    char  fnmeta[L_tmpnam];
    
    tmpnam(fnbody);
    tmpnam(fnmeta);
    
    fp = fopen(fnbody,"w");
    if (fp != NULL)
    {
      fputs(req->body,fp);
      fclose(fp);
    }
    else
    {
      syslog(LOG_ERR,"entry_add: tmp-body: %s",strerror(errno));
      return false;
    }
    
    fp = fopen(fnmeta,"w");
    if (fp != NULL)
    {
      char ts[12];
      
      if (req->date == NULL)
        snprintf(ts,sizeof(ts),"%04d/%02d/%02d",blog->now.year,blog->now.month,blog->now.day);
      else
        snprintf(ts,sizeof(ts),"%s",req->date);
        
      fprintf(
        fp,
        "Author: %s\n"
        "Title: %s\n"
        "Class: %s\n"
        "Status: %s\n"
        "Date: %s\n"
        "Adtag: %s\n"
        "\n",
        req->author,
        req->title,
        req->class,
        req->status,
        ts,
        req->adtag
      );
      fclose(fp);
    }
    else
    {
      remove(fnbody);
      syslog(LOG_ERR,"entry_add: tmp-meta: %s",strerror(errno));
      return false;
    }
    
    rc = run_hook("entry-pre-hook",(char const *[]){ blog->config.prehook , fnbody , fnmeta , NULL });
    
    remove(fnmeta);
    remove(fnbody);
    
    if (!rc)
      return rc;
  }
  
  entry = BlogEntryNew(blog);
  
  if (emptynull_string(req->date))
    entry->when = blog->now;
  else
  {
    char *p;
    
    entry->when.year  = strtoul(req->date,&p,10); p++;
    entry->when.month = strtoul(p,        &p,10); p++;
    entry->when.day   = strtoul(p,        &p,10);
  }
  
  entry->when.part = 0;
  entry->timestamp = blog->tnow;
  entry->title     = req->title;
  entry->class     = req->class;
  entry->status    = req->status;
  entry->author    = req->author;
  entry->adtag     = req->adtag;
  entry->body      = req->body;
  
  BlogEntryWrite(entry);
  
  req->when = entry->when;
  
  if (blog->config.posthook != NULL)
  {
    char const *argv[6];
    char        url[1024];
    
    snprintf(
      url,
      sizeof(url),
      "%s/%04d/%02d/%02d.%d",
      blog->config.url,
      entry->when.year,
      entry->when.month,
      entry->when.day,
      entry->when.part
    );
    
    argv[0] = blog->config.posthook;
    argv[1] = url;
    argv[2] = req->title;
    argv[3] = req->author;
    argv[4] = req->status;
    argv[5] = NULL;
    
    rc = run_hook("entry-post-hook",argv);
  }
  
  free(entry);
  return rc;
}

/************************************************************************/

void fix_entry(Request *req)
{
  FILE   *out;
  FILE   *in;
  char   *tmp;
  size_t  size;
  
  assert(req != NULL);
  
  /*---------------------------------
  ; convert the title
  ;--------------------------------*/
  
  if (!empty_string(req->title))
  {
    tmp  = NULL;
    size = 0;
    out  = open_memstream(&tmp,&size);
    in   = fmemopen(req->title,strlen(req->title),"r");
    
    buff_conversion(in,out);
    fclose(in);
    fclose(out);
    free(req->title);
    req->title = entity_conversion(tmp);
    free(tmp);
  }
  
  /*--------------------------------------
  ; convert the status
  ;-------------------------------------*/
  
  if (!empty_string(req->status))
  {
    tmp = NULL;
    size = 0;
    out  = open_memstream(&tmp,&size);
    in   = fmemopen(req->status,strlen(req->status),"r");
    
    buff_conversion(in,out);
    fclose(in);
    fclose(out);
    free(req->status);
    req->status = entity_conversion(tmp);
    free(tmp);
  }
  
  /*-------------------------------------
  ; convert body
  ;--------------------------------------*/
  
  if (!empty_string(req->body))
  {
    tmp  = NULL;
    size = 0;
    out  = open_memstream(&tmp,&size);
    in   = fmemopen(req->body,strlen(req->body),"r");
    
    fcopy(out,in);
    fclose(in);
    fclose(out);
    free(req->body);
    req->body = tmp;
  }
}

/*************************************************************************/
