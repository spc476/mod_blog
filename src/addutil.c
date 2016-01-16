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

#define _GNU_SOURCE	1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include <cgilib6/nodelist.h>
#include <cgilib6/htmltok.h>
#include <cgilib6/util.h>
#include <cgilib6/pair.h>
#include <cgilib6/cgi.h>
#include <cgilib6/mail.h>

#include "conf.h"
#include "blog.h"
#include "conversion.h"
#include "timeutil.h"
#include "frontend.h"
#include "backend.h"
#include "globals.h"
#include "fix.h"
#include "blogutil.h"

#ifdef EMAIL_NOTIFY
#  include <gdbm.h>
#endif

/*********************************************************************/

int entry_add(Request req)
{
  BlogEntry  entry;
  char      *p;
  
  assert(req != NULL);
  
  fix_entry(req);
  
  entry = BlogEntryNew(g_blog);
  
  if ((req->date == NULL) || (empty_string(req->date)))
  {
    /*----------------------------------------------------------------------
    ; if this is the case, then we need to ensure we update the main pages. 
    ; By calling BlogDatesInit() after we update we ensure we generate the
    ; content properly.
    ;---------------------------------------------------------------------*/
    
    entry->when = g_blog->now;
  }
  else
  {
    entry->when.year  = strtoul(req->date,&p,10); p++;
    entry->when.month = strtoul(p,        &p,10); p++;
    entry->when.day   = strtoul(p,        &p,10);
  }
  
  entry->when.part = 0;
  entry->timestamp = g_blog->tnow;
  entry->title     = req->title;
  entry->class     = req->class;
  entry->status    = req->status;
  entry->author    = req->author;
  entry->body      = req->body;
  
  if (c_authorfile) BlogLock(g_blog);

    BlogEntryWrite(entry);

  if (c_authorfile) BlogUnlock(g_blog);
  
  free(entry);
  return(0);    
}

/************************************************************************/

void fix_entry(Request req)
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
  
    (*c_conversion)(in,out);
    fclose(in);
    fclose(out);
    free(req->body);
    req->body = tmp;
  }
}

/************************************************************************/

void notify_emaillist(void)
{
#ifdef EMAIL_NOTIFY
  GDBM_FILE  list;
  datum      key;
  datum      content;
  Email      email;
  FILE      *in;
 
  list = gdbm_open((char *)c_emaildb,DB_BLOCK,GDBM_READER,0,dbcritical);
  if (list == NULL)
    return;
 
  email          = EmailNew();
  email->from    = c_email;
  email->subject = c_emailsubject;
  
  in = fopen(c_emailmsg,"r");
  if (in == NULL)
  {
    EmailFree(email);
    gdbm_close(list);
    return;
  }
  
  fcopy(email->body,in);
  fclose(in);
  
  key = gdbm_firstkey(list);
 
  while(key.dptr != NULL)
  {
    content = gdbm_fetch(list,key);
    if (content.dptr != NULL)
    {
      email->to = content.dptr;
      EmailSend(email);
    }
    key = gdbm_nextkey(list,key);
  }

  email->to = NULL;
  EmailFree(email);
  gdbm_close(list);
#endif
}

/*************************************************************************/

void dbcritical(const char *msg)
{
  if (msg)
    fprintf(stderr,"critical error: %s\n",msg);
}

/*************************************************************************/

