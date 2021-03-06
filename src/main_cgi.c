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

#ifndef __GNUC__
#  define __attribute__(x)
#endif

#include <stdlib.h>
#include <string.h>

#include <syslog.h>
#include <cgilib6/cgi.h>
#include <cgilib6/util.h>

#include "frontend.h"
#include "globals.h"

/**************************************************************************/

static int  cgi_init               (Cgi,Request *);
static void set_m_cgi_get_command  (char *,Request *);
static int  cmd_cgi_get_new        (Request *);
static int  cmd_cgi_get_show       (Request *);
static int  cmd_cgi_get_edit       (Request *);
static int  cmd_cgi_get_overview   (Request *);
static int  cmd_cgi_get_today      (Request *);
static void set_m_cgi_post_command (char *,Request *);
static void set_m_author           (char *,Request *);
static int  cmd_cgi_post_new       (Request *);
static int  cmd_cgi_post_show      (Request *);
static int  cmd_cgi_post_edit      (Request *);
static int  cgi_error              (Request *,int,char const *, ... );

/*************************************************************************/

int main_cgi_head(Cgi cgi)
{
  int rc;
  
  assert(cgi != NULL);
  
  rc = cgi_init(cgi,&gd.req);
  if (rc != 0)
    return (*gd.req.error)(&gd.req,HTTP_ISERVERERR,"cgi_init() failed");
    
  return (*gd.req.error)(&gd.req,HTTP_METHODNOTALLOWED,"HEAD method not supported");
}

/**********************************************************************/

int main_cgi_get(Cgi cgi)
{
  int rc;
  
  assert(cgi != NULL);
  
  rc = cgi_init(cgi,&gd.req);
  if (rc != 0)
    return (*gd.req.error)(&gd.req,HTTP_ISERVERERR,"cgi_init() failed");
    
  gd.req.command    = cmd_cgi_get_show;
  gd.req.reqtumbler = getenv("PATH_INFO");
  
  CgiListMake(cgi);
  
  set_m_cgi_get_command(CgiListGetValue(cgi,"cmd"),&gd.req);
  
  rc = (*gd.req.command)(&gd.req);
  return rc;
}

/************************************************************************/

static void set_m_cgi_get_command(char *value,Request *req)
{
  assert(req != NULL);
  
  if (emptynull_string(value)) return;
  up_string(value);
  
  if (strcmp(value,"NEW") == 0)
    req->command = cmd_cgi_get_new;
  else if (strcmp(value,"SHOW") == 0)
    req->command = cmd_cgi_get_show;
  else if (strcmp(value,"PREVIEW") == 0)
    req->command = cmd_cgi_get_show;
  else if (strcmp(value,"EDIT") == 0)
    req->command = cmd_cgi_get_edit;
  else if (strcmp(value,"OVERVIEW") == 0)
    req->command = cmd_cgi_get_overview;
  else if (strcmp(value,"TODAY") == 0)
    req->command = cmd_cgi_get_today;
}

/***********************************************************************/

static int cmd_cgi_get_new(Request *req)
{
  struct callback_data cbd;
  
  assert(req != NULL);
  
  memset(&cbd,0,sizeof(struct callback_data));
  ListInit(&cbd.list);
  cbd.adtag = (char *)c_adtag;
  gd.f.edit = 1;
  fputs("Status: 200\r\nContent-type: text/html\r\n\r\n",req->out);
  generic_cb("main",req->out,&cbd);
  return 0;
}

/**********************************************************************/

static int cmd_cgi_get_show(Request *req)
{
  char *status;
  int   rc = -1;
  
  assert(req != NULL);
  
  status = CgiListGetValue(gd.cgi,"status");
  if (emptynull_string(status))
    status = strdup("200");
    
  if ((emptynull_string(req->reqtumbler)) || (strcmp(req->reqtumbler,"/") == 0))
  {
    fprintf(
        req->out,
        "Status: %d\r\n"
        "Location: %s\r\n"
        "Content-type: text/html\r\n"
        "\r\n"
        "<HTML>\n"
        "  <HEAD><TITLE>Go here</TITLE></HEAD>\n"
        "  <BODY><A HREF='%s'>Go here<A></BODY>\n"
        "</HTML>\n",
        HTTP_MOVEPERM,
        c_fullbaseurl,
        c_fullbaseurl
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
                req->out,
                "Status: %d\r\n"
                "Location: %s/%s\r\n"
                "Content-Type: text/html\r\n\r\n"
                "<html>"
                "<head><title>Redirect</title></head>"
                "<body><p>Redirect to <a href='%s/%s'>%s/%s</a></p></body>"
                "</html>\n",
                HTTP_MOVEPERM,
                c_fullbaseurl, tum,
                c_fullbaseurl, tum,
                c_fullbaseurl, tum
        );
        free(tum);
        free(status);
        return 0;
      }
      
      if (req->tumbler.file == false)
        fprintf(
                req->out,
                "Status: %s\r\n"
                "Content-type: text/html\r\n\r\n",
                status
        );
      rc = tumbler_page(req->out,&req->tumbler);
    }
    else
    {
      FILE *in;
      
      in   = fopen(getenv("PATH_TRANSLATED"),"r");
      if (in == NULL)
        rc = (*req->error)(req,HTTP_BADREQ,"bad request");
      else
      {
        gd.htmldump = in;
        fprintf(req->out,"Status: %s\r\nContent-type: text/html\r\n\r\n",status);
        generic_cb("main",req->out,NULL);
        rc = 0;
      }
    }
  }
  
  free(status);
  assert(rc != -1);
  return rc;
}

/********************************************************************/

static int cmd_cgi_get_edit(Request *req)
{
  assert(req != NULL);
  return (*req->error)(req,HTTP_BADREQ,"bad request");
}

/***********************************************************************/

static int cmd_cgi_get_overview(Request *req)
{
  List days;
  assert(req != NULL);
  
  ListInit(&days);
  gd.f.overview = true;
  fprintf(req->out,"Status: %d\r\nContent-type: text/html\r\n\r\n",HTTP_OKAY);
  generic_cb("main",req->out,&days);
  return 0;
}

/**********************************************************************/

static int cmd_cgi_get_today(Request *req)
{
  assert(req != NULL);
  
  char *tpath = CgiListGetValue(gd.cgi,"path");
  char *twhen = CgiListGetValue(gd.cgi,"day");
  
  if ((tpath == NULL) && (twhen == NULL))
  {
    fprintf(req->out,"Status: %d\r\nContent-type: text/html\r\n\r\n",HTTP_OKAY);
    return generate_thisday(req->out,g_blog->now);
  }
    
  if (tpath == NULL)
    return (*req->error)(req,HTTP_BADREQ,"bad request");
  
  if ((twhen == NULL) || (*twhen == '\0'))
  {
    fprintf(
      req->out,
      "Status: %d\r\n"
      "Content-Type: text/html\r\n"
      "Location: %s/%s\r\n"
      "\r\n"
      "<HTML>\n"
      "  <HEAD><TITLE>Go here</TITLE></HEAD>\n"
      "  <BODY><A HREF='%s/%s'>Go here</A></BODY>\n"
      "</HTML>\n",
      HTTP_MOVEPERM,
      c_fullbaseurl,tpath,
      c_fullbaseurl,tpath
    );
    return 0;
  }
  
  if (!thisday_new(&req->tumbler,twhen))
    return (*req->error)(req,HTTP_BADREQ,"bad request");
  
  if (req->tumbler.redirect)
  {
    fprintf(
      req->out,
      "Status: %d\r\n"
      "Content-Type: text/html\r\n"
      "Location: %s/%s/%02d/%02d\r\n"
      "\r\n"
      "<HTML>\n"
      "  <HEAD><TITLE>Go here</TITLE></HEAD>\n"
      "  <BODY><A HREF='%s/%s/%02d/%02d'>Go here</A></BODY>\n"
      "</HTML>\n",
      HTTP_MOVEPERM,
      c_fullbaseurl,tpath,req->tumbler.start.month,req->tumbler.start.day,
      c_fullbaseurl,tpath,req->tumbler.start.month,req->tumbler.start.day
    );
    return 0;
  }
  
  fprintf(req->out,"Status: %d\r\nContent-type: text/html\r\n\r\n",HTTP_OKAY);
  return generate_thisday(req->out,req->tumbler.start);
}

/**********************************************************************/

int main_cgi_post(Cgi cgi)
{
  int rc;
  
  assert(cgi != NULL);
  
  rc = cgi_init(cgi,&gd.req);
  if (rc != 0)
    return (*gd.req.error)(&gd.req,HTTP_ISERVERERR,"cgi_init() failed");
    
  gd.req.command = cmd_cgi_post_new;
  
  CgiListMake(cgi);
  
  set_c_updatetype      (CgiListGetValue(cgi,"updatetype"));
  set_cf_emailupdate    (CgiListGetValue(cgi,"email"));
  set_c_conversion      (CgiListGetValue(cgi,"filter"));
  set_m_author          (CgiListGetValue(cgi,"author"),&gd.req);
  set_m_cgi_post_command(CgiListGetValue(cgi,"cmd"),&gd.req);
  
  gd.req.title    = strdup(CgiListGetValue(cgi,"title"));
  gd.req.class    = strdup(CgiListGetValue(cgi,"class"));
  gd.req.status   = strdup(CgiListGetValue(cgi,"status"));
  gd.req.date     = strdup(CgiListGetValue(cgi,"date"));
  gd.req.adtag    = strdup(CgiListGetValue(cgi,"adtag"));
  gd.req.origbody = strdup(CgiListGetValue(cgi,"body"));
  gd.req.body     = strdup(gd.req.origbody);
  
  if (
       (emptynull_string(gd.req.author))
       || (emptynull_string(gd.req.title))
       || (emptynull_string(gd.req.body))
     )
  {
    return (*gd.req.error)(&gd.req,HTTP_BADREQ,"errors-missing");
  }
  
  if (gd.req.class == NULL)
    gd.req.class = strdup("");
    
  if (authenticate_author(&gd.req) == false)
  {
    syslog(LOG_ERR,"'%s' not authorized to post",gd.req.author);
    return (*gd.req.error)(&gd.req,HTTP_UNAUTHORIZED,"errors-author not authenticated got [%s] wanted [%s]",gd.req.author,CgiListGetValue(cgi,"author"));
  }
  
  rc = (*gd.req.command)(&gd.req);
  return rc;
}

/************************************************************************/

static void set_m_cgi_post_command(char *value,Request *req)
{
  assert(req != NULL);
  
  if (emptynull_string(value)) return;
  up_string(value);
  
  if (strcmp(value,"NEW") == 0)
    req->command = cmd_cgi_post_new;
  else if (strcmp(value,"SHOW") == 0)
    req->command = cmd_cgi_post_show;
  else if (strcmp(value,"EDIT") == 0)
    req->command = cmd_cgi_post_edit;
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

static int cmd_cgi_post_new(Request *req)
{
  int rc;
  
  assert(req != NULL);
  
  rc = entry_add(req);
  if (rc == 0)
  {
    if (cf_emailupdate) notify_emaillist(req);
    generate_pages();
    fprintf(
        req->out,
        "Status: %d\r\n"
        "Location: %s\r\n"
        "Content-type: text/html\r\n"
        "\r\n"
        "<HTML>\n"
        "  <HEAD><TITLE>Go here</TITLE></HEAD>\n"
        "  <BODY><A HREF='%s'>Go here</A></BODY>\n"
        "</HTML>\n",
        HTTP_MOVETEMP,
        c_fullbaseurl,
        c_fullbaseurl
        
    );
  }
  else
    rc = (*req->error)(req,HTTP_ISERVERERR,"couldn't add entry");
    
  return rc;
}

/***********************************************************************/

static int cmd_cgi_post_show(Request *req)
{
  struct callback_data cbd;
  BlogEntry           *entry;
  char                *p;
  
  assert(req != NULL);
  
  /*----------------------------------------------------
  ; this routine is a mixture between entry_add() and
  ; tumbler_page().  Perhaps some refactoring is in order
  ; at some point.
  ;-------------------------------------------------------*/
  
  memset(&cbd,0,sizeof(struct callback_data));
  ListInit(&cbd.list);
  cbd.adtag = (char *)c_adtag;
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
  gd.f.edit = 1;
  fputs("Status: 200\r\nContent-type: text/html\r\n\r\n",req->out);
  generic_cb("main",req->out,&cbd);
  
  return 0;
}

/**********************************************************************/

static int cmd_cgi_post_edit(Request *req)
{
  assert(req != NULL);
  
  return (req->error)(req,HTTP_BADREQ,"bad request");
}

/***********************************************************************/

static int cgi_error(Request *req,int level,char const *msg, ... )
{
  FILE    *in;
  va_list  args;
  char    *file   = NULL;
  char    *errmsg = NULL;
  
  assert(req   != NULL);
  assert(level >= 0);
  assert(msg   != NULL);
  
  va_start(args,msg);
  vasprintf(&errmsg,msg,args);
  va_end(args);
  
  asprintf(&file,"%s/errors/%d.html",getenv("DOCUMENT_ROOT"),level);
  
  in = fopen(file,"r");
  if (in == NULL)
  {
    fprintf(
        req->out,
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
    gd.htmldump = in;
    fprintf(
        req->out,
        "Status: %d\r\n"
        "X-Error: %s\r\n"
        "Content-type: text/html\r\n"
        "\r\n",
        level,
        errmsg
      );
    generic_cb("main",req->out,NULL);
    fclose(in);
  }
  
  free(file);
  free(errmsg);
  
  return 0;
}

/**********************************************************************/

static int cgi_init(Cgi cgi,Request *req)
{
  assert(cgi != NULL);
  assert(req != NULL);
  
  req->error = cgi_error;
  req->in    = stdin;
  req->out   = stdout;
  gd.cgi     = cgi;
  
  
  return GlobalsInit(NULL);
}

/***********************************************************************/
