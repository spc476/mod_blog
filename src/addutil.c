
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

#include <gdbm.h>

#include <cgil/memory.h>
#include <cgil/buffer.h>
#include <cgil/ddt.h>
#include <cgil/clean.h>
#include <cgil/errors.h>
#include <cgil/nodelist.h>
#include <cgil/htmltok.h>
#include <cgil/util.h>
#include <cgil/pair.h>
#include <cgil/cgi.h>

#include "conf.h"
#include "blog.h"
#include "conversion.h"
#include "timeutil.h"
#include "doi_util.h"
#include "frontend.h"
#include "globals.h"
#include "fix.h"

/***********************************************************************/

void fix_entry(Request req)
{
  Buffer in;
  Buffer lin;
  Buffer out;
  char   tmpbuf[65536UL];
  int    rc;
 
  ddt(req != NULL);
  
  memset(tmpbuf,0,sizeof(tmpbuf));
  rc = MemoryBuffer(&out,tmpbuf,sizeof(tmpbuf));
  rc = MemoryBuffer(&in,req->title,strlen(req->title));
  BufferIOCtl(in,CM_SETSIZE,strlen(req->title));
  LineBuffer(&lin,in);
  buff_conversion(lin,out,QUOTE_SMART);

  BufferFree(&lin);
  BufferFree(&in); 
  BufferFree(&out);
 
  MemFree(req->title,strlen(req->title) + 1);
  req->title = dup_string(tmpbuf);

  memset(tmpbuf,0,sizeof(tmpbuf));
  rc = MemoryBuffer(&out,tmpbuf,sizeof(tmpbuf));
  rc = MemoryBuffer(&in,req->body,strlen(req->body));
  BufferIOCtl(in,CM_SETSIZE,strlen(req->body));
 
  (*g_conversion)("body",in,out);

  BufferFree(&in); 
  BufferFree(&out);
 
  MemFree(req->body,strlen(req->body) + 1);
  req->body = dup_string(tmpbuf);
}

/************************************************************************/

void collect_body(Buffer output,Buffer input)
{
  HtmlToken token;
  int       rc;
  int       t;

  ddt(output != NULL);
  ddt(input  != NULL);
 
  rc = HtmlParseNew(&token,input);
  if (rc != ERR_OKAY)
  {
    ErrorPush(AppErr,CBBODY,rc,"");
    ErrorLog();
    ErrorClear();
    return;
  }
 
  while((t = HtmlParseNext(token)) != T_EOF)
  {
    if (t == T_TAG)
    {
      struct pair *pp;
 
      if (strcmp(HtmlParseValue(token),"/HTML") == 0) break;
      if (strcmp(HtmlParseValue(token),"/BODY") == 0) break;
      BufferFormatWrite(output,"$","<%a",HtmlParseValue(token));
      for (
            pp = HtmlParseFirstOption(token);
            NodeValid(&pp->node);
            pp = (struct pair *)NodeNext(&pp->node)
          )
      {
        if (pp->name && !empty_string(pp->name))
          BufferFormatWrite(output,"$"," %a",pp->name);
        if (pp->value && !empty_string(pp->value))
          BufferFormatWrite(output,"$","=\"%a\"",pp->value);
      }
      BufferFormatWrite(output,"",">");
    }
    else if (t == T_STRING)
    {
      size_t s = strlen(HtmlParseValue(token));
      BufferWrite(output,HtmlParseValue(token),&s);
    }
    else if (t == T_COMMENT)
    {
      BufferFormatWrite(output,"$","<!%a>",HtmlParseValue(token));
    }
  }

  HtmlParseFree(&token);    
}

/***********************************************************************/

static char *headers[] = 
{
  "",
  "User-agent: mod_blog/R13 (addentry-1.22 http://boston.conman.org/about)\r\n",
  "Accept: text/html\r\n",
  "\r\n",
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
  char     buffer[BUFSIZ];
  size_t   size;
  size_t   fsize;
  int      fh;

  fsize      = 6 + strlen(g_email) + 2 + 1;
  headers[0] = MemAlloc(fsize);
  sprintf(headers[0],"From: %s\r\n",g_email);

  name    = UrlEncodeString(g_name);
  blogurl = UrlEncodeString(g_fullbaseurl);

  size = 5	/* name = */
         + strlen(name)
         + 1	/* & */
         + 4	/* url= */
         + strlen(blogurl)
         + 1;	/* EOS */

  query = MemAlloc(size);
  sprintf(query,"name=%s&url=%s",name,blogurl);

  UrlNew((URL *)&url,g_weblogcomurl);
 
  nfile = MemAlloc(strlen(url->file) + 1 + strlen(query) + 1);
  sprintf(nfile,"%s?%s",url->file,query);
  MemFree(url->file,strlen(url->file) + 1);
  MemFree(query,size);
  url->file = nfile;
 
  rc = HttpOpen(&conn,GET,url,(const char **)headers);
  if (rc != ERR_OKAY)
  {
    ErrorClear();
    return;
  }
 
  BufferIOCtl(HttpBuffer(conn),CF_HANDLE,&fh);
 
  while(read(fh,buffer,sizeof(buffer)) > 0)
    ;
  HttpClose(&conn);
  UrlFree((URL *)&url);
  MemFree(headers[0],fsize);
}

/*************************************************************************/

void notify_emaillist(void)
{
  GDBM_FILE list;
  datum     key;
  datum     content;
 
  list = gdbm_open((char *)g_emaildb,DB_BLOCK,GDBM_READER,0,dbcritical);
  if (list == NULL)
    return;
 
  key = gdbm_firstkey(list);
 
  while(key.dptr != NULL)
  {
    content = gdbm_fetch(list,key);
    if (content.dptr != NULL)
    {
      send_message(g_email,NULL,content.dptr,g_emailsubject,g_emailmsg);
    }
    key = gdbm_nextkey(list,key);
  }
  gdbm_close(list);
}

/*************************************************************************/

void dbcritical(char *msg)
{
  if (msg)
    ddtlog(ddtstream,"$","critical error [%a]",msg);
}

/*************************************************************************/

