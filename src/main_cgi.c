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
#include <errno.h>

#include <syslog.h>
#include <cgilib7/util.h>

#include "backend.h"
#include "frontend.h"
#include "conversion.h"
#include "main.h"

typedef int (*cgicmd__f)(Cgi,Blog *,struct request *);

/************************************************************************/

static int cgi_error(Blog *blog,Request *request,int level,char const *msg, ... )
{
  va_list  args;
  char    *file   = NULL;
  char    *errmsg = NULL;
  
  assert(level >= 400);
  assert(msg   != NULL);
  
  va_start(args,msg);
  vasprintf(&errmsg,msg,args);
  va_end(args);
  
  if (level >= HTTP_ISERVERERR)
    syslog(LOG_ERR,"%d: %s",level,errmsg);
    
  asprintf(&file,"%s/errors/%d.html",getenv("DOCUMENT_ROOT"),level);
  
  if ((blog == NULL) || (freopen(file,"r",stdin) == NULL))
  {
    char buf[BUFSIZ];
    int  len = snprintf(
                 buf,
                 sizeof(buf),
                 "<html lang='en'>\n"
                 "<head>\n"
                 "<title>Error %d</title>\n"
                 "</head>\n"
                 "<body>\n"
                 "<h1>Error %d</h1>\n"
                 "<p>%s</p>\n"
                 "</body>\n"
                 "</html>\n"
                 "",
                 level,
                 level,
                 errmsg
               );
    printf(
      "Status: %d\r\n"
      "Content-Type: text/html\r\n"
      "Content-Length: %d\r\n"
      "\r\n"
      "%s"
      "",
      level,
      len,
      buf
    );
  }
  else
  {
    struct callback_data cbd;
    
    assert(blog    != NULL);
    assert(request != NULL);
    assert(request->f.cgi);
    
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

static int cmd_cgi_error(Cgi cgi,Blog *blog,Request *request)
{
  (void)cgi;
  return cgi_error(blog,request,HTTP_BADREQ,"Bad Request");
}

/**********************************************************************/

static int cmd_cgi_get_new(Cgi cgi,Blog *blog,Request *req)
{
  struct callback_data cbd;
  
  (void)cgi;
  (void)blog;
  assert(req  != NULL);
  
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
  
  status = CgiGetQValue(cgi,"status");
  if (!emptynull_string(status))
  {
    int level = strtoul(status,NULL,10);
    if (level >= 400)
      return cgi_error(blog,req,level,"Error from Apache");
  }
  
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
  
  assert(rc != -1);
  return rc;
}

/********************************************************************/

static int cmd_cgi_get_today(Cgi cgi,Blog *blog,Request *req)
{
  assert(cgi  != NULL);
  assert(blog != NULL);
  assert(req  != NULL);
  
  char *tpath = CgiGetQValue(cgi,"path");
  char *twhen = CgiGetQValue(cgi,"day");
  
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

static int cmd_cgi_get_last(Cgi cgi,Blog *blog,Request *req)
{
  char buf[BUFSIZ];
  int  len;
  
  assert(cgi  != NULL);
  assert(blog != NULL);
  assert(req  != NULL);
  
  char *date = CgiGetQValue(cgi,"date");
  if (date == NULL)
  {
    len = snprintf(
                 buf,
                 sizeof(buf),
                 "%s%04d/%02d/%02d.%d",
                 blog->config.url,
                 blog->last.year,
                 blog->last.month,
                 blog->last.day,
                 blog->last.part
               );
  }
  else
  {
    struct btm  when;
    char       *p;
    size_t      last;
    
    when.year  = strtoul(date,&p,10); p++;
    when.month = strtoul(p,   &p,10); p++;
    when.day   = strtoul(p,   &p,10);
    when.part  = 1;
    
    if (btm_cmp(&when,&blog->first) < 0)
      return cgi_error(blog,req,HTTP_BADREQ,"date out of range");
    if (btm_cmp(&when,&blog->last) <= 0)
      last = BlogLastEntry(blog,&when);
    else
      last = 0;
      
    len  = snprintf(
                  buf,
                  sizeof(buf),
                  "%s%04d/%02d/%02d.%zu",
                  blog->config.url,
                  when.year,
                  when.month,
                  when.day,
                  last
            );
  }
  
  printf(
      "Status: %d\r\n"
      "Content-Type: text/plain\r\n"
      "Content-Length: %d\r\n"
      "\r\n"
      "%s",
      HTTP_OKAY,
      len,
      buf
  );
  
  return 0;
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
  else if (strcmp(value,"last") == 0)
    return cmd_cgi_get_last;
  else
    return cmd_cgi_error;
}

/***********************************************************************/

static void set_m_author(char *value,Request *req)
{
  assert(req != NULL);
  
  if (emptynull_string(value))
    req->author = get_remote_user();
  else
    req->author = safe_strdup(value);
    
  req->origauthor = strdup(req->author);
}

/************************************************************************/

static int cmd_cgi_post_new(Cgi cgi,Blog *blog,Request *req)
{
  (void)cgi;
  assert(blog != NULL);
  assert(req  != NULL);
  
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
  
  (void)cgi;
  assert(blog != NULL);
  assert(req  != NULL);
  
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
  entry->title     = strdup(req->title);
  entry->class     = strdup(req->class);
  entry->status    = strdup(req->status);
  entry->author    = strdup(req->author);
  entry->body      = strdup(req->body);
  
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
    return cmd_cgi_error;
}

/************************************************************************/

int main_cgi_bad(Cgi cgi)
{
  (void)cgi;
  return cgi_error(NULL,NULL,HTTP_METHODNOTALLOWED,"Nope, not allowed.");
}

/*************************************************************************/

int main_cgi_GET(Cgi cgi)
{
  assert(cgi != NULL);
  
  Request  request;
  Blog    *blog = BlogNew(NULL);
  
  if (blog == NULL)
    return cgi_error(NULL,NULL,HTTP_ISERVERERR,"Could not instantiate the blog");
    
  request_init(&request);
  request.f.cgi      = true;
  request.reqtumbler = getenv("PATH_INFO");
  
  if (CgiStatus(cgi) != HTTP_OKAY)
    cgi_error(blog,&request,CgiStatus(cgi),"processing error");
  else
    (*set_m_cgi_get_command(CgiGetQValue(cgi,"cmd")))(cgi,blog,&request);
    
  BlogFree(blog);
  request_free(&request);
  return 0;
}

/************************************************************************/

int main_cgi_POST(Cgi cgi)
{
  assert(cgi != NULL);
  
  Request  request;
  Blog    *blog = BlogNew(NULL);
  
  if (blog == NULL)
    return cgi_error(NULL,NULL,HTTP_ISERVERERR,"Could not instantiate the blog");
    
  request_init(&request);
  request.f.cgi = true;
  
  if (CgiStatus(cgi) != HTTP_OKAY)
  {
    cgi_error(blog,&request,CgiStatus(cgi),"processing error");
    goto cleanup_return;
  }
  
  set_m_author(CgiGetValue(cgi,"author"),&request);
  
  request.title      = safe_strdup(CgiGetValue(cgi,"title"));
  request.class      = safe_strdup(CgiGetValue(cgi,"class"));
  request.status     = safe_strdup(CgiGetValue(cgi,"status"));
  request.date       = safe_strdup(CgiGetValue(cgi,"date"));
  request.adtag      = safe_strdup(CgiGetValue(cgi,"adtag"));
  request.origbody   = safe_strdup(CgiGetValue(cgi,"body"));
  request.body       = safe_strdup(request.origbody);
  request.conversion = TO_conversion(CgiGetValue(cgi,"filter"),blog->config.conversion);
  request.f.email    = TO_email(CgiGetValue(cgi,"email"),blog->config.email.notify);
  
  if (
          (emptynull_string(request.author))
       || (emptynull_string(request.title))
       || (emptynull_string(request.body))
     )
  {
    cgi_error(blog,&request,HTTP_BADREQ,"errors-missing");
    goto cleanup_return;
  }
  
  if (authenticate_author(blog,&request) == false)
  {
    syslog(LOG_ERR,"'%s' not authorized to post",request.author);
    cgi_error(blog,&request,HTTP_UNAUTHORIZED,"errors-author not authenticated got [%s] wanted [%s]",request.author,CgiGetValue(cgi,"author"));
  }
  else
    (*set_m_cgi_post_command(CgiGetValue(cgi,"cmd")))(cgi,blog,&request);
    
cleanup_return:
  BlogFree(blog);
  request_free(&request);
  return 0;
}

/************************************************************************/

int main_cgi_PUT(Cgi cgi)
{
  assert(cgi != NULL);
  
  if (CgiStatus(cgi) != HTTP_OKAY)
    return cgi_error(NULL,NULL,CgiStatus(cgi),"processing error");
    
  Blog *blog = BlogNew(NULL);
  
  if (blog == NULL)
    return cgi_error(NULL,NULL,HTTP_ISERVERERR,"Could not instantiate the blog");
    
  if (getenv("HTTP_BLOG_FILE") == NULL)
  {
    Request  request;
    
    request_init(&request);
    set_m_author(getenv("HTTP_BLOG_AUTHOR"),&request);
    
    request.title      = safe_strdup(getenv("HTTP_BLOG_TITLE"));
    request.class      = safe_strdup(getenv("HTTP_BLOG_CLASS"));
    request.status     = safe_strdup(getenv("HTTP_BLOG_STATUS"));
    request.date       = safe_strdup(getenv("HTTP_BLOG_DATE"));
    request.adtag      = safe_strdup(getenv("HTTP_BLOG_ADTAG"));
    request.conversion = TO_conversion(getenv("HTTP_BLOG_FILTER"),blog->config.conversion);
    request.f.email    = TO_email(getenv("HTTP_BLOG_EMAIL"),blog->config.email.notify);
    request.body       = malloc(CgiContentLength(cgi) + 1);
    
    fread(request.body,1,CgiContentLength(cgi),stdin);
    request.body[CgiContentLength(cgi)] = '\0';
    
    if (
            (emptynull_string(request.author))
         || (emptynull_string(request.title))
         || (emptynull_string(request.body))
      )
    {
      cgi_error(NULL,NULL,HTTP_BADREQ,"errors-missing");
      goto cleanup_return;
    }
    
    if (!authenticate_author(blog,&request))
    {
      syslog(LOG_ERR,"'%s' not authorized to post",request.author);
      cgi_error(NULL,NULL,HTTP_UNAUTHORIZED,"errors-author not authenticatged got [%s] wanted [%s]",request.author,getenv("HTTP_BLOG_AUTHOR"));
      goto cleanup_return;
    }
    
    if (entry_add(blog,&request))
    {
      generate_pages(blog,&request);
      printf("Status: %d\r\n\r\n",HTTP_NOCONTENT);
    }
    else
      cgi_error(NULL,NULL,HTTP_ISERVERERR,"couldn't add entry");
      
cleanup_return:
    request_free(&request);
    BlogFree(blog);
    return 0;
  }
  else
  {
    char        filename[FILENAME_MAX];
    size_t      bytes;
    FILE       *fp;
    char const *path = getenv("PATH_INFO");
    
    if (path == NULL)
    {
      cgi_error(NULL,NULL,HTTP_ISERVERERR,"couldn't add file");
      goto cleanup_file_return;
    }
    
    snprintf(filename,sizeof(filename),"%s%s",blog->config.basedir,path);
    fp = fopen(filename,"wb");
    if (fp == NULL)
    {
      cgi_error(NULL,NULL,HTTP_ISERVERERR,"%s: %s",path,strerror(errno));
      goto cleanup_file_return;
    }
      
    do
    {
      char buffer[BUFSIZ];
      
      bytes = fread(buffer,1,sizeof(buffer),stdin);
      fwrite(buffer,1,bytes,fp);
    } while (bytes > 0);
    
    fclose(fp);
    printf("Status: %d\r\n\r\n",HTTP_NOCONTENT);
cleanup_file_return:
    BlogFree(blog);
    return 0;
  }
}

/************************************************************************/
