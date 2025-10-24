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
#include <cgilib8/util.h>

#include "backend.h"

/*********************************************************************/

http__e entry_add(Blog *blog,Request *req)
{
  BlogEntry *entry;
  http__e    status;
  
  assert(blog != NULL);
  assert(req  != NULL);
  
  if (blog->config.prehook != NULL)
  {
    FILE *fp;
    char  fnbody[L_tmpnam];
    char  fnmeta[L_tmpnam];
    bool  rc;
    
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
      return HTTP_ISERVERERR;
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
      return HTTP_ISERVERERR;
    }
    
    rc = run_hook("entry-pre-hook",(char const *[]){ blog->config.prehook , fnbody , fnmeta , NULL });
    
    remove(fnmeta);
    remove(fnbody);
    
    if (!rc)
      return HTTP_UNPROCESSENTITY;
  }
  
  entry = BlogEntryNew(blog);
  if (entry == NULL)
    return HTTP_ISERVERERR;
  
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
  
  if (BlogEntryWrite(entry) == 0)
  {
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
      
      if (!run_hook("entry-post-hook",argv))
        status = HTTP_ACCEPTED;
      else
        status = HTTP_CREATED;
    }
    else
      status = HTTP_CREATED;
  }
  else
    status = HTTP_ISERVERERR;
    
  /*-----------------------------------------------------------------------------
  ; DO NOT CALL BlogFreeEntry() here!  It is NOT The Right Thing To Do(TM) here!
  ; The fields of entry are not allocated, but are merely referenced from
  ; other structures, so they don't need freeing; only the base structure
  ; itself needs to be freed.
  ;------------------------------------------------------------------------------*/
  
  free(entry);
  return status;
}

/************************************************************************/
