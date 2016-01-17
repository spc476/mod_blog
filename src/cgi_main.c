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

#define _GNU_SOURCE 1

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#include <syslog.h>
#include <cgilib6/cgi.h>
#include <cgilib6/util.h>

#include "conf.h"
#include "blog.h"
#include "frontend.h"
#include "backend.h"
#include "timeutil.h"
#include "wbtum.h"
#include "fix.h"
#include "globals.h"

/**************************************************************************/

static int	cgi_init		(Cgi,Request);

static void	set_m_cgi_get_command	(char *const,Request);
static int	cmd_cgi_get_new		(Request);
static int	cmd_cgi_get_show	(Request);
static int	cmd_cgi_get_edit	(Request);
static int	cmd_cgi_get_overview	(Request);

static void	set_m_cgi_post_command	(char *const,Request);
static void	set_m_author		(char *const,Request);
static int	cmd_cgi_post_new	(Request);
static int	cmd_cgi_post_show	(Request);
static int	cmd_cgi_post_edit	(Request);

static int	cgi_error		(Request,int,const char *, ... );

/*************************************************************************/

int main_cgi_head(Cgi cgi)
{
  struct request req;
  int            rc;
  
  assert(cgi != NULL);
  
  rc = cgi_init(cgi,&req);
  if (rc != 0)
    return((*req.error)(&req,HTTP_ISERVERERR,"cgi_init() failed"));
  
  return((*req.error)(&req,HTTP_METHODNOTALLOWED,"HEAD method not supported"));
}

/**********************************************************************/

int main_cgi_get(Cgi cgi)
{
  struct request  req;
  int             rc;
  
  assert(cgi != NULL);

  rc = cgi_init(cgi,&req);
  if (rc != 0)
    return((*req.error)(&req,HTTP_ISERVERERR,"cgi_init() failed"));

  req.command    = cmd_cgi_get_show;
  req.reqtumbler = getenv("PATH_INFO");
  gd.req         = &req;
  
  CgiListMake(cgi);
  
  set_m_cgi_get_command(CgiListGetValue(cgi,"cmd"),&req);
  
  rc = (*req.command)(&req);
  gd.req = NULL;	/* make sure we don't have a dangling reference */
  return(rc);
}

/************************************************************************/

static void set_m_cgi_get_command(char *value,Request req)
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
}

/***********************************************************************/

static int cmd_cgi_get_new(Request req)
{
  struct callback_data cbd;

  assert(req != NULL);
  
  memset(&cbd,0,sizeof(struct callback_data));
  ListInit(&cbd.list);
  cbd.adtag = (char *)c_adtag;
  gd.f.edit = 1;
  fputs("Status: 200\r\nContent-type: text/html\r\n\r\n",req->out);
  generic_cb("main",req->out,&cbd);
  return(0);
}

/**********************************************************************/

static int cmd_cgi_get_show(Request req)
{
  char *status;
  int   rc = -1;
  
  assert(req != NULL);
  
  status = CgiListGetValue(req->cgi,"status");
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
    if (tumbler_new(&req->tumbler,req->reqtumbler))
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
	return(0);
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
  return(rc);
}

/********************************************************************/

static int cmd_cgi_get_edit(Request req)
{
  assert(req != NULL);
  return((*req->error)(req,HTTP_BADREQ,"bad request"));
}

/***********************************************************************/

static int cmd_cgi_get_overview(Request req)
{
  List days;
  assert(req != NULL);
  
  ListInit(&days);
  gd.f.overview = true;
  fprintf(req->out,"Status: %d\r\nContent-type: text/html\r\n\r\n",HTTP_OKAY);
  generic_cb("main",req->out,&days);
  return(0);  
}

/**********************************************************************/

int main_cgi_post(Cgi cgi)
{
  struct request req;
  int            rc;
  
  assert(cgi != NULL);
  
  rc = cgi_init(cgi,&req);
  if (rc != 0)
    return((*req.error)(&req,HTTP_ISERVERERR,"cgi_init() failed"));
  
  req.command = cmd_cgi_post_new;  
  gd.req      = &req;
  
  CgiListMake(cgi);
  
  set_c_updatetype 	(CgiListGetValue(cgi,"updatetype"));
  set_cf_emailupdate	(CgiListGetValue(cgi,"email"));
  set_c_conversion 	(CgiListGetValue(cgi,"filter"));
  set_m_author     	(CgiListGetValue(cgi,"author"),&req);
  set_m_cgi_post_command(CgiListGetValue(cgi,"cmd"),&req);
  
  req.title     = CgiListGetValue(cgi,"title");
  req.class     = CgiListGetValue(cgi,"class");
  req.status    = CgiListGetValue(cgi,"status");
  req.date      = CgiListGetValue(cgi,"date");
  req.origbody  = CgiListGetValue(cgi,"body");
  req.body      = strdup(req.origbody);

  if (
       (emptynull_string(req.author))
       || (emptynull_string(req.title))
       || (emptynull_string(req.body))
     )
  {
    gd.req = NULL; /* no dangling references */
    return((*req.error)(&req,HTTP_BADREQ,"errors-missing"));
  }

  if (req.class == NULL)
    req.class = strdup("");
  
  if (authenticate_author(&req) == false)
  {
    gd.req = NULL;
    syslog(LOG_ERR,"'%s' not authorized to post",req.author);
    return((*req.error)(&req,HTTP_UNAUTHORIZED,"errors-author not authenticated got [%s] wanted [%s]",req.author,CgiListGetValue(cgi,"author")));
  }

  rc = (*req.command)(&req);
  gd.req = NULL;
  return(rc);  
}

/************************************************************************/

static void set_m_cgi_post_command(char *value,Request req)
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

static void set_m_author(char *value,Request req)
{
  assert(req != NULL);
  
  if (emptynull_string(value))
    req->author = get_remote_user();
  else
    req->author = strdup(value);
    
  req->origauthor = strdup(req->author);
}

/************************************************************************/

static int cmd_cgi_post_new(Request req)
{
  int rc;
  
  assert(req != NULL);
  
  rc = entry_add(req);
  if (rc == 0)
  {
    if (cf_emailupdate) notify_emaillist();
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
  
  return(rc);
}

/***********************************************************************/

static int cmd_cgi_post_show(Request req)
{
  struct callback_data cbd;
  BlogEntry            entry;
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
  
  entry->when.part = 1;	/* doesn't matter what this is */
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

  return(0);  
}

/**********************************************************************/

static int cmd_cgi_post_edit(Request req)
{
  assert(req != NULL);
  
  return((req->error)(req,HTTP_BADREQ,"bad request"));
}

/***********************************************************************/

static int cgi_error(Request req,int level,const char *msg, ... )
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
  
  return(0);
}

/**********************************************************************/

static int cgi_init(Cgi cgi,Request req)
{
  assert(cgi != NULL);
  assert(req != NULL);
  
  memset(req,0,sizeof(struct request));
  
  req->error = cgi_error;
  req->in    = stdin;
  req->out   = stdout;
  req->cgi   = cgi;
  
  return GlobalsInit(NULL);
}

/***********************************************************************/

