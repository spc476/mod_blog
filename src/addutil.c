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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/stream.h>
#include <cgilib/errors.h>
#include <cgilib/nodelist.h>
#include <cgilib/htmltok.h>
#include <cgilib/util.h>
#include <cgilib/pair.h>
#include <cgilib/cgi.h>
#include <cgilib/mail.h>

#include "conf.h"
#include "blog.h"
#include "conversion.h"
#include "timeutil.h"
#include "frontend.h"
#include "globals.h"
#include "fix.h"

#ifdef EMAIL_NOTIFY
#  include <gdbm.h>
#endif

/*********************************************************************/

int entry_add(Request req)
{
  BlogEntry  entry;
  char      *p;
  
  fix_entry(req);
  if (c_authorfile) BlogLock(g_blog);
  
  entry = BlogEntryNew(g_blog);
  
  if ((req->date == NULL) || (empty_string(req->date)))
    entry->when = gd.updatetime;
  else
  {
    entry->when.year  = strtoul(req->date,&p,10); p++;
    entry->when.month = strtoul(p,        &p,10); p++;
    entry->when.day   = strtoul(p,        &p,10);
  }
  
  entry->when.part = 0;
  entry->timestamp = gd.tst;
  entry->title     = req->title;
  entry->class     = req->class;
  entry->author    = req->author;
  entry->body      = req->body;
  
  BlogEntryWrite(entry);
  
  if (c_authorfile) BlogUnlock(g_blog);
  return(ERR_OKAY);    
}

/************************************************************************/

void fix_entry(Request req)
{
  Stream in;
  Stream out;
  
  out = StringStreamWrite();
  
  /*-------------------
  ; convert the title
  ;--------------------*/
  
  in  = MemoryStreamRead(req->title,strlen(req->title));
  buff_conversion(in,out,QUOTE_SMART);
  StreamFree(in);
  MemFree(req->title);
  req->title = StringFromStream(out);
  
  StreamFlush(out);
  
  /*--------------
  ; convert body
  ;----------------*/
  
  in = MemoryStreamRead(req->body,strlen(req->body));
  (*c_conversion)("body",in,out);
  StreamFree(in);
  MemFree(req->body);
  req->body = StringFromStream(out);
  
  StreamFree(out);
}

/************************************************************************/

static char *headers[] = 
{
  "",
  "User-agent: mod_blog/R14 (addentry-2.0 http://boston.conman.org/about)",
  "Accept: text/html",
  NULL
};

void notify_weblogcom(void)
{
  URLHTTP url;
  HTTP    conn;
  char    *query;
  char    *name;
  char    *blogurl;
  char    *nfile;
  int      rc;
  int      c;
  size_t   size;
  size_t   fsize;

  fsize      = 6 + strlen(c_email) + 2 + 1;
  headers[0] = MemAlloc(fsize);
  sprintf(headers[0],"From: %s\r\n",c_email);

  name    = UrlEncodeString(c_name);
  blogurl = UrlEncodeString(c_fullbaseurl);

  size = 5	/* name = */
         + strlen(name)
         + 1	/* & */
         + 4	/* url= */
         + strlen(blogurl)
         + 1;	/* EOS */

  query = MemAlloc(size);
  sprintf(query,"name=%s&url=%s",name,blogurl);

  UrlNew((URL *)&url,c_weblogcomurl);
 
  nfile = MemAlloc(strlen(url->file) + 1 + strlen(query) + 1);
  sprintf(nfile,"%s?%s",url->file,query);
  MemFree(url->file);
  MemFree(query);
  url->file = nfile;
 
  rc = HttpOpen(&conn,GET,url,(const char **)headers);
  if (rc != ERR_OKAY)
    return;

  while(!StreamEOF(HttpStreamRead(conn)))
    c = StreamRead(HttpStreamRead(conn));
  
  HttpClose(&conn);
  UrlFree((URL *)&url);
  MemFree(headers[0]);
}

/*************************************************************************/

void notify_emaillist(void)
{
#ifdef EMAIL_NOTIFY
  GDBM_FILE list;
  datum     key;
  datum     content;
  Email     email;
  Stream    in;
 
  list = gdbm_open((char *)c_emaildb,DB_BLOCK,GDBM_READER,0,dbcritical);
  if (list == NULL)
    return;
 
  email          = EmailNew();
  email->from    = c_email;
  email->subject = c_emailsubject;
  
  in = FileStreamRead(c_emailmsg);
  if (in == NULL)
  {
    EmailFree(email);
    gdbm_close(list);
    return;
  }
  
  StreamCopy(email->body,in);
  StreamFree(in);
  
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

void dbcritical(char *msg)
{
  if (msg)
    ddtlog(ddtstream,"$","critical error [%a]",msg);
}

/*************************************************************************/

