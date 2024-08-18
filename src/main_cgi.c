/************************************************************************
*
* Copyright 2005 by Sean Conner.  All Rights Reserved.
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
*************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <syslog.h>
#include <cgilib6/util.h>

#include "backend.h"
#include "frontend.h"
#include "conversion.h"
#include "main.h"

typedef int (*cgicmd__f)(Cgi,Blog *,struct request *);

/**************************************************************************/

static int cgi_error(Blog *blog,Request *request,int level,char const *msg, ... )
{
  va_list  args;
  char    *file   = NULL;
  char    *errmsg = NULL;
  
  assert(level >= 0);
  assert(msg   != NULL);
  
  va_start(args,msg);
  vasprintf(&errmsg,msg,args);
  va_end(args);
  
  if (level >= HTTP_ISERVERERR)
    syslog(LOG_ERR,"%d: %s",level,errmsg);
  
  asprintf(&file,"%s/errors/%d.html",getenv("DOCUMENT_ROOT"),level);
  
  if ((blog == NULL) || (freopen(file,"r",stdin) == NULL))
  {
    fprintf(
        stdout,
        "Status: %d\r\n"
        "X-Error: %s\r\n"
        "Content-type: text/html\r\n"
        "\r\n"
        "<html>\n"
        "<head>\n"
        "<title>Error %d</title>\n"
        "</head>\n"
        "<body>\n"
        "<h1>Error %d</h1>\n"
        "<p>%s</p>\n"
        "</body>\n"
        "</html>\n"
        "\n",
        level,
        errmsg,
        level,
        level,
        errmsg
    );
  }
  else
  {
    struct callback_data cbd;
    
    assert(blog    != NULL);
    assert(request != NULL);
    
    request->f.htmldump = true;
    callback_init(&cbd,blog,request);
    cbd.status = level;
    generic_main(stdout,&cbd);
  }
  
  free(file);
  free(errmsg);
  
  return 0;
}

/**********************************************************************/

static void redirect(http__e status,char const *base,char const *path)
{
  assert((status == HTTP_MOVEPERM) || (status == HTTP_MOVETEMP));
  assert(base != NULL);
  assert(path != NULL);
  
  char doc[BUFSIZ];
  int  len = snprintf(
               doc,
               sizeof(doc),
                "<HTML>\n"
                "  <HEAD><TITLE>Go here</TITLE></HEAD>\n"
                "  <BODY><A HREF='%s%s'>%s%s</A></BODY>\n"
                "</HTML>\n",
                base,path,
                base,path
              );
  printf(
    "Status: %d\r\n"
    "Location: %s%s\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: %d\r\n"
    "\r\n"
    "%s",
    status,
    base, path,
    len,
    doc
  );
}

/**********************************************************************/

static int cmd_cgi_get_new(Cgi cgi,Blog *blog,Request *req)
{
  struct callback_data cbd;
  
  assert(cgi  != NULL);
  assert(blog != NULL);
  assert(req  != NULL);
  (void)cgi;
  (void)blog;
  
  req->f.edit = true;
  generic_main(stdout,callback_init(&cbd,blog,req));
  return 0;
}

/**********************************************************************/

static int cmd_cgi_get_show(Cgi cgi,Blog *blog,Request *req)
{
  char *status;
  int   rc = -1;
  
  assert(cgi  != NULL);
  assert(blog != NULL);
  assert(req  != NULL);
  
  status = CgiListGetValue(cgi,"status");
  if (emptynull_string(status))
    status = strdup("200");
    
  if ((emptynull_string(req->reqtumbler)) || (strcmp(req->reqtumbler,"/") == 0))
  {
    redirect(HTTP_MOVEPERM,blog->config.url,"");
    rc = 0;
  }
  else
  {
    req->reqtumbler++;
    if (tumbler_new(&req->tumbler,req->reqtumbler,&blog->first,&blog->last))
    {
      if (req->tumbler.redirect)
      {
        char *tum = tumbler_canonical(&req->tumbler);
        redirect(HTTP_MOVEPERM,blog->config.url,tum);
        free(tum);
        free(status);
        return 0;
      }
      
      rc = tumbler_page(blog,req,&req->tumbler,cgi_error);
    }
    else
    {
      char filename[FILENAME_MAX];
      
      snprintf(filename,sizeof(filename),"%s%s",getenv("DOCUMENT_ROOT"),getenv("PATH_INFO"));
      if (freopen(filename,"r",stdin) == NULL)
        rc = cgi_error(blog,req,HTTP_NOTFOUND,"bad request");
      else
      {
        struct callback_data cbd;
        
        req->f.htmldump = true;
        generic_main(stdout,callback_init(&cbd,blog,req));
        rc = 0;
      }
    }
  }
  
  free(status);
  assert(rc != -1);
  return rc;
}

/********************************************************************/

static int cmd_cgi_get_today(Cgi cgi,Blog *blog,Request *req)
{
  assert(cgi  != NULL);
  assert(blog != NULL);
  assert(req  != NULL);
  
  char *tpath = CgiListGetValue(cgi,"path");
  char *twhen = CgiListGetValue(cgi,"day");
  
  if ((tpath == NULL) && (twhen == NULL))
  {
    return generate_thisday(blog,req,stdout,blog->now);
  }
  
  if (tpath == NULL)
    return cgi_error(blog,req,HTTP_BADREQ,"bad request");
    
  if ((twhen == NULL) || (*twhen == '\0'))
  {
    redirect(HTTP_MOVEPERM,blog->config.url,tpath);
    return 0;
  }
  
  if (!thisday_new(&req->tumbler,twhen))
    return cgi_error(blog,req,HTTP_BADREQ,"bad request");
    
  if (req->tumbler.redirect)
  {
    char buf[BUFSIZ];
    snprintf(
      buf,
      sizeof(buf),
      "%s/%02d/%02d",
      tpath,req->tumbler.start.month,req->tumbler.start.day
    );
    redirect(HTTP_MOVEPERM,blog->config.url,buf);
    return 0;
  }
  
  return generate_thisday(blog,req,stdout,req->tumbler.start);
}

/**********************************************************************/

static cgicmd__f set_m_cgi_get_command(char const *value)
{
  if (emptynull_string(value))
    return cmd_cgi_get_show;
  else if (strcmp(value,"new") == 0)
    return cmd_cgi_get_new;
  else if (strcmp(value,"show") == 0)
    return cmd_cgi_get_show;
  else if (strcmp(value,"preview") == 0)
    return cmd_cgi_get_show;
  else if (strcmp(value,"today") == 0)
    return cmd_cgi_get_today;
  else
  {
    syslog(LOG_WARNING,"'%s' not supported, using 'show'",value);
    return cmd_cgi_get_show;
  }
}

/***********************************************************************/

static void set_m_author(char *value,Request *req)
{
  assert(req != NULL);
  
  if (emptynull_string(value))
    req->author = get_remote_user();
  else
    req->author = value;
    
  req->origauthor = strdup(req->author);
}

/************************************************************************/

static int cmd_cgi_post_new(Cgi cgi,Blog *blog,Request *req)
{
  assert(cgi  != NULL);
  assert(blog != NULL);
  assert(req  != NULL);
  (void)cgi;
  
  if (entry_add(blog,req))
  {
    generate_pages(blog,req);
    redirect(HTTP_MOVETEMP,blog->config.url,"");
    return 0;
  }
  else
    return cgi_error(blog,req,HTTP_ISERVERERR,"couldn't add entry");
}

/***********************************************************************/

static int cmd_cgi_post_show(Cgi cgi,Blog *blog,Request *req)
{
  struct callback_data cbd;
  BlogEntry           *entry;
  
  assert(cgi  != NULL);
  assert(blog != NULL);
  assert(req  != NULL);
  (void)cgi;
  
  /*-----------------------------------------------------------------
  ; this routine is a mixture between entry_add() and tumbler_page().
  ; Perhaps some refactoring is in order at some point.
  ;------------------------------------------------------------------*/
  
  callback_init(&cbd,blog,req);
  fix_entry(req);
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
  
  entry->when.part = 1; /* doesn't matter what this is */
  entry->timestamp = blog->tnow;
  entry->title     = req->title;
  entry->class     = req->class;
  entry->status    = req->status;
  entry->author    = req->author;
  entry->body      = req->body;
  
  ListAddTail(&cbd.list,&entry->node);
  req->f.edit = true;
  generic_main(stdout,&cbd);
  
  return 0;
}

/**********************************************************************/

static cgicmd__f set_m_cgi_post_command(char const *value)
{
  assert(value != NULL);
  
  if (emptynull_string(value))
    return cmd_cgi_post_show;
  else if (strcmp(value,"new") == 0)
    return cmd_cgi_post_new;
  else if (strcmp(value,"show") == 0)
    return cmd_cgi_post_show;
  else
  {
    syslog(LOG_WARNING,"'%s' not supported, using 'show'",value);
    return cmd_cgi_post_show;
  }
}

/************************************************************************/

void request_free(Request *request)
{
  assert(request != NULL);
  
  free(request->origauthor);
  free(request->author);
  free(request->title);
  free(request->class);
  free(request->status);
  free(request->date);
  free(request->adtag);
  free(request->origbody);
  free(request->body);
}

/*************************************************************************/

int main_cgi_bad(Cgi cgi)
{
  (void)cgi;
  return cgi_error(NULL,NULL,HTTP_METHODNOTALLOWED,"Nope, now allowed.");
}

/*************************************************************************/

int main_cgi_GET(Cgi cgi)
{
  assert(cgi != NULL);
  
  Request  request;
  Blog    *blog = BlogNew(NULL);
  
  if (blog == NULL)
    return cgi_error(NULL,NULL,HTTP_ISERVERERR,"Could not instantiate the blog");
    
  if (cgi->status != HTTP_OKAY)
    return cgi_error(blog,&request,cgi->status,"");
    
  CgiListMake(cgi);
  request_init(&request);
  request.f.cgi      = true;
  request.reqtumbler = getenv("PATH_INFO");
  int rc = (*set_m_cgi_get_command(CgiListGetValue(cgi,"cmd")))(cgi,blog,&request);
  BlogFree(blog);
  request_free(&request);
  return rc;
}

/************************************************************************/

static char *safe_strdup(char const *orig)
{
  return orig != NULL ? strdup(orig) : NULL;
}

/************************************************************************/

int main_cgi_POST(Cgi cgi)
{
  assert(cgi != NULL);
  
  Request  request;
  Blog    *blog = BlogNew(NULL);
  
  if (blog == NULL)
    return cgi_error(NULL,NULL,HTTP_ISERVERERR,"Could not instantiate the blog");
    
  if (cgi->status != HTTP_OKAY)
    return cgi_error(blog,&request,cgi->status,"");
    
  CgiListMake(cgi);
  request_init(&request);
  set_m_author(CgiListGetValue(cgi,"author"),&request);
  
  request.title      = safe_strdup(CgiListGetValue(cgi,"title"));
  request.class      = safe_strdup(CgiListGetValue(cgi,"class"));
  request.status     = safe_strdup(CgiListGetValue(cgi,"status"));
  request.date       = safe_strdup(CgiListGetValue(cgi,"date"));
  request.adtag      = safe_strdup(CgiListGetValue(cgi,"adtag"));
  request.origbody   = safe_strdup(CgiListGetValue(cgi,"body"));
  request.body       = safe_strdup(request.origbody);
  request.conversion = TO_conversion(CgiListGetValue(cgi,"filter"),blog->config.conversion);
  request.f.email    = TO_email(CgiListGetValue(cgi,"email"),blog->config.email.notify);
  request.f.cgi      = true;
  
  if (
          (emptynull_string(request.author))
       || (emptynull_string(request.title))
       || (emptynull_string(request.body))
     )
  {
    return cgi_error(blog,&request,HTTP_BADREQ,"errors-missing");
  }
  
  if (request.class == NULL)
    request.class = strdup("");
    
  if (authenticate_author(blog,&request) == false)
  {
    syslog(LOG_ERR,"'%s' not authorized to post",request.author);
    return cgi_error(blog,&request,HTTP_UNAUTHORIZED,"errors-author not authenticated got [%s] wanted [%s]",request.author,CgiListGetValue(cgi,"author"));
  }
  
  int rc = (*set_m_cgi_post_command(CgiListGetValue(cgi,"cmd")))(cgi,blog,&request);
  BlogFree(blog);
  request_free(&request);
  return rc;
}

/************************************************************************/
