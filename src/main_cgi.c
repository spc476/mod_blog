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
#include "globals.h"

typedef int (*cgicmd__f)(Cgi,struct request *);

/**************************************************************************/

static bool      cgi_init               (Cgi);
static cgicmd__f set_m_cgi_get_command  (char const *);
static int       cmd_cgi_get_new        (Cgi,Request *);
static int       cmd_cgi_get_show       (Cgi,Request *);
static int       cmd_cgi_get_today      (Cgi,Request *);
static cgicmd__f set_m_cgi_post_command (char const *);
static void      set_m_author           (char *,Request *);
static int       cmd_cgi_post_new       (Cgi,Request *);
static int       cmd_cgi_post_show      (Cgi,Request *);
static int       cgi_error              (int,char const *, ... );

/*************************************************************************/

int main_cgi_get(Cgi cgi)
{
  assert(cgi != NULL);
  
  if (!cgi_init(cgi))
    return cgi_error(HTTP_ISERVERERR,"cgi_init() failed");
    
  g_request.reqtumbler = getenv("PATH_INFO");
  return (*set_m_cgi_get_command(CgiListGetValue(cgi,"cmd")))(cgi,&g_request);
}

/************************************************************************/

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

static int cmd_cgi_get_new(Cgi cgi,Request *req)
{
  struct callback_data cbd;
  
  assert(req != NULL);
  assert(cgi != NULL);
  (void)cgi;
  
  req->f.edit   = true;
  fputs("Status: 200\r\nContent-type: text/html\r\n\r\n",stdout);
  generic_cb("main",stdout,callback_init(&cbd));
  return 0;
}

/**********************************************************************/

static int cmd_cgi_get_show(Cgi cgi,Request *req)
{
  char *status;
  int   rc = -1;
  
  assert(req != NULL);
  assert(cgi != NULL);
  
  status = CgiListGetValue(cgi,"status");
  if (emptynull_string(status))
    status = strdup("200");
    
  if ((emptynull_string(req->reqtumbler)) || (strcmp(req->reqtumbler,"/") == 0))
  {
    fprintf(
        stdout,
        "Status: %d\r\n"
        "Location: %s\r\n"
        "Content-type: text/html\r\n"
        "\r\n"
        "<HTML>\n"
        "  <HEAD><TITLE>Go here</TITLE></HEAD>\n"
        "  <BODY><A HREF='%s'>Go here<A></BODY>\n"
        "</HTML>\n",
        HTTP_MOVEPERM,
        c_config->url,
        c_config->url
      );
    rc = 0;
  }
  else
  {
    req->reqtumbler++;
    if (tumbler_new(&req->tumbler,req->reqtumbler,&g_blog->first,&g_blog->last))
    {
    
      if (req->tumbler.redirect)
      {
        char *tum = tumbler_canonical(&req->tumbler);
        fprintf(
                stdout,
                "Status: %d\r\n"
                "Location: %s/%s\r\n"
                "Content-Type: text/html\r\n\r\n"
                "<html>"
                "<head><title>Redirect</title></head>"
                "<body><p>Redirect to <a href='%s/%s'>%s/%s</a></p></body>"
                "</html>\n",
                HTTP_MOVEPERM,
                c_config->url, tum,
                c_config->url, tum,
                c_config->url, tum
        );
        free(tum);
        free(status);
        return 0;
      }
      
      if (req->tumbler.file == false)
        fprintf(
                stdout,
                "Status: %s\r\n"
                "Content-type: text/html\r\n\r\n",
                status
        );
      rc = tumbler_page(&req->tumbler,cgi_error);
    }
    else
    {
      char filename[FILENAME_MAX];
      
      snprintf(filename,sizeof(filename),"%s%s",getenv("DOCUMENT_ROOT"),getenv("PATH_INFO"));
      if (freopen(filename,"r",stdin) == NULL)
        rc = cgi_error(HTTP_BADREQ,"bad request");
      else
      {
        struct callback_data cbd;
        
        req->f.htmldump = true;
        fprintf(stdout,"Status: %s\r\nContent-type: text/html\r\n\r\n",status);
        generic_cb("main",stdout,callback_init(&cbd));
        rc = 0;
      }
    }
  }
  
  free(status);
  assert(rc != -1);
  return rc;
}

/********************************************************************/

static int cmd_cgi_get_today(Cgi cgi,Request *req)
{
  assert(req != NULL);
  assert(cgi != NULL);
  
  char *tpath = CgiListGetValue(cgi,"path");
  char *twhen = CgiListGetValue(cgi,"day");
  
  if ((tpath == NULL) && (twhen == NULL))
  {
    fprintf(stdout,"Status: %d\r\nContent-type: text/html\r\n\r\n",HTTP_OKAY);
    return generate_thisday(stdout,g_blog->now);
  }
  
  if (tpath == NULL)
    return cgi_error(HTTP_BADREQ,"bad request");
    
  if ((twhen == NULL) || (*twhen == '\0'))
  {
    fprintf(
      stdout,
      "Status: %d\r\n"
      "Content-Type: text/html\r\n"
      "Location: %s/%s\r\n"
      "\r\n"
      "<HTML>\n"
      "  <HEAD><TITLE>Go here</TITLE></HEAD>\n"
      "  <BODY><A HREF='%s/%s'>Go here</A></BODY>\n"
      "</HTML>\n",
      HTTP_MOVEPERM,
      c_config->url,tpath,
      c_config->url,tpath
    );
    return 0;
  }
  
  if (!thisday_new(&req->tumbler,twhen))
    return cgi_error(HTTP_BADREQ,"bad request");
    
  if (req->tumbler.redirect)
  {
    fprintf(
      stdout,
      "Status: %d\r\n"
      "Content-Type: text/html\r\n"
      "Location: %s/%s/%02d/%02d\r\n"
      "\r\n"
      "<HTML>\n"
      "  <HEAD><TITLE>Go here</TITLE></HEAD>\n"
      "  <BODY><A HREF='%s/%s/%02d/%02d'>Go here</A></BODY>\n"
      "</HTML>\n",
      HTTP_MOVEPERM,
      c_config->url,tpath,req->tumbler.start.month,req->tumbler.start.day,
      c_config->url,tpath,req->tumbler.start.month,req->tumbler.start.day
    );
    return 0;
  }
  
  fprintf(stdout,"Status: %d\r\nContent-type: text/html\r\n\r\n",HTTP_OKAY);
  return generate_thisday(stdout,req->tumbler.start);
}

/**********************************************************************/

int main_cgi_post(Cgi cgi)
{
  assert(cgi != NULL);
  
  if (cgi_init(cgi) != 0)
    return cgi_error(HTTP_ISERVERERR,"cgi_init() failed");
    
  set_m_author(CgiListGetValue(cgi,"author"),&g_request);
  
  g_request.title      = strdup(CgiListGetValue(cgi,"title"));
  g_request.class      = strdup(CgiListGetValue(cgi,"class"));
  g_request.status     = strdup(CgiListGetValue(cgi,"status"));
  g_request.date       = strdup(CgiListGetValue(cgi,"date"));
  g_request.adtag      = strdup(CgiListGetValue(cgi,"adtag"));
  g_request.origbody   = strdup(CgiListGetValue(cgi,"body"));
  g_request.body       = strdup(g_request.origbody);
  g_request.conversion = TO_conversion(CgiListGetValue(cgi,"filter"),c_config->conversion);
  g_request.f.email    = TO_email(CgiListGetValue(cgi,"email"));
  
  if (
       (emptynull_string(g_request.author))
       || (emptynull_string(g_request.title))
       || (emptynull_string(g_request.body))
     )
  {
    return cgi_error(HTTP_BADREQ,"errors-missing");
  }
  
  if (g_request.class == NULL)
    g_request.class = strdup("");
    
  if (authenticate_author(&g_request,c_config) == false)
  {
    syslog(LOG_ERR,"'%s' not authorized to post",g_request.author);
    return cgi_error(HTTP_UNAUTHORIZED,"errors-author not authenticated got [%s] wanted [%s]",g_request.author,CgiListGetValue(cgi,"author"));
  }
  
  return (*set_m_cgi_post_command(CgiListGetValue(cgi,"cmd")))(cgi,&g_request);
}

/************************************************************************/

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

static int cmd_cgi_post_new(Cgi cgi,Request *req)
{
  assert(req != NULL);
  assert(cgi != NULL);
  (void)cgi;
  
  if (entry_add(req,c_config,g_blog))
  {
    if (req->f.email) notify_emaillist(req,c_config);
    generate_pages();
    fprintf(
        stdout,
        "Status: %d\r\n"
        "Location: %s\r\n"
        "Content-type: text/html\r\n"
        "\r\n"
        "<HTML>\n"
        "  <HEAD><TITLE>Go here</TITLE></HEAD>\n"
        "  <BODY><A HREF='%s'>Go here</A></BODY>\n"
        "</HTML>\n",
        HTTP_MOVETEMP,
        c_config->url,
        c_config->url
        
    );
    return 0;
  }
  else
    return cgi_error(HTTP_ISERVERERR,"couldn't add entry");
}

/***********************************************************************/

static int cmd_cgi_post_show(Cgi cgi,Request *req)
{
  struct callback_data cbd;
  BlogEntry           *entry;
  char                *p;
  
  assert(req != NULL);
  assert(cgi != NULL);
  (void)cgi;
  
  /*----------------------------------------------------
  ; this routine is a mixture between entry_add() and
  ; tumbler_page().  Perhaps some refactoring is in order
  ; at some point.
  ;-------------------------------------------------------*/
  
  callback_init(&cbd);
  fix_entry(req);
  entry = BlogEntryNew(g_blog);
  
  if (emptynull_string(req->date))
    entry->when = g_blog->now;
  else
  {
    entry->when.year  = strtoul(req->date,&p,10); p++;
    entry->when.month = strtoul(p,        &p,10); p++;
    entry->when.day   = strtoul(p,        &p,10);
  }
  
  entry->when.part = 1; /* doesn't matter what this is */
  entry->timestamp = g_blog->tnow;
  entry->title     = req->title;
  entry->class     = req->class;
  entry->status    = req->status;
  entry->author    = req->author;
  entry->body      = req->body;
  
  ListAddTail(&cbd.list,&entry->node);
  req->f.edit = true;
  fputs("Status: 200\r\nContent-type: text/html\r\n\r\n",stdout);
  generic_cb("main",stdout,&cbd);
  
  return 0;
}

/**********************************************************************/

static int cgi_error(int level,char const *msg, ... )
{
  va_list  args;
  char    *file   = NULL;
  char    *errmsg = NULL;
  
  assert(level >= 0);
  assert(msg   != NULL);
  
  va_start(args,msg);
  vasprintf(&errmsg,msg,args);
  va_end(args);
  
  asprintf(&file,"%s/errors/%d.html",getenv("DOCUMENT_ROOT"),level);
  
  if (freopen(file,"r",stdin) == NULL)
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
    
    g_request.f.htmldump = true;

    fprintf(
        stdout,
        "Status: %d\r\n"
        "X-Error: %s\r\n"
        "Content-type: text/html\r\n"
        "\r\n",
        level,
        errmsg
      );
    generic_cb("main",stdout,callback_init(&cbd));
  }
  
  free(file);
  free(errmsg);
  
  return 0;
}

/**********************************************************************/

static bool cgi_init(Cgi cgi)
{
  assert(cgi != NULL);
  
  g_request.f.cgi = true;
  CgiListMake(cgi);
  return GlobalsInit(NULL);
}

/***********************************************************************/
