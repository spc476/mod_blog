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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <cgilib/memory.h>
#include <cgilib/errors.h>
#include <cgilib/ddt.h>
#include <cgilib/cgi.h>
#include <cgilib/util.h>

#include "conf.h"
#include "globals.h"
#include "blog.h"
#include "frontend.h"
#include "timeutil.h"
#include "stream.h"
#include "wbtum.h"
#include "fix.h"

/**************************************************************************/

static int	cgi_init		(Cgi,Request);

static void	set_m_cgi_get_command	(char *,Request);
static int	cmd_cgi_get_new		(Request);
static int	cmd_cgi_get_show	(Request);
static int	cmd_cgi_get_edit	(Request);

static void	set_m_cgi_post_command	(char *,Request);
static void	set_m_author		(char *,Request);
static int	cmd_cgi_post_new	(Request);
static int	cmd_cgi_post_show	(Request);
static int	cmd_cgi_post_edit	(Request);

static int	cgi_error		(Request,int,char *,char *, ... );

/*************************************************************************/

int main_cgi_get(Cgi cgi,int argc,char *argv[])
{
  struct request  req;
  int             rc;
  
  ddt(cgi != NULL);

  rc = cgi_init(cgi,&req);
  if (rc != ERR_OKAY)
    return((*req.error)(&req,HTTP_ISERVERERR,"","cgi_init() failed"));

  req.command    = cmd_cgi_get_show;
  req.reqtumbler = CgiEnvGet(cgi,"PATH_INFO");
  gd.req         = &req;
  
  CgiListMake(cgi);
  
  set_m_cgi_get_command(CgiListGetValue(cgi,"cmd"),&req);
  
  rc = (*req.command)(&req);
  return(rc);
}

/************************************************************************/

static void set_m_cgi_get_command(char *value,Request req)
{
  ddt(req != NULL);
  
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
}

/***********************************************************************/

static int cmd_cgi_get_new(Request req)
{
  List days;

  ddt(req != NULL);
  
  ListInit(&days);
  gd.f.edit = 1;
  LineS(req->out,"Status: 200\r\nContent-type: text/html\r\n\r\n");
  generic_cb("main",req->out,&days);
  return(0);
}

/**********************************************************************/

static int cmd_cgi_get_show(Request req)
{
  char *status;
  int   rc;
  
  ddt(req != NULL);
  
  status = CgiListGetValue(req->cgi,"status");
  if (emptynull_string(status))
    status = dup_string("200");
  
  if ((empty_string(req->reqtumbler)) || (strcmp(req->reqtumbler,"/") == 0))
  {
    LineSFormat(
    	req->out,
    	"$",
	"Status: %a\r\n"
    	"Content-type: text/html\r\n"
    	"\r\n",
    	status
    );
    rc = generate_pages(req);
  }
  else
  {
    req->reqtumbler++;
    rc = TumblerNew(&req->tumbler,&req->reqtumbler);
    if (rc == ERR_OKAY)
    {

#ifdef FEATURE_REDIRECT
      if (req->tumbler->flags.redirect)
      {
        char *tum = TumblerCanonical(req->tumbler);
        LineSFormat(
        	req->out,
        	"i $ $",
        	"Status: %a\r\n"
        	"Location: %b/%c\r\n"
        	"Content-Type: text/html\r\n\r\n"
        	"<html>"
        	"<head><title>Redirect</title></head>"
        	"<body><p>Redirect to <a href='%b/%c'>%b/%c</a></p></body>"
        	"</html>\n",
        	HTTP_MOVEPERM,
        	c_fullbaseurl,
        	tum
        );
        MemFree(tum);
	MemFree(status);
	return(ERR_OKAY);
      }
#endif

      if (req->tumbler->flags.file == FALSE)
        LineSFormat(
        	req->out,
        	"$",
        	"Status: %a\r\n"
        	"Content-type: text/html\r\n\r\n",
        	status
        );
      rc = tumbler_page(req->out,req->tumbler);
    }
    else
    {
      Stream  in;
      char   *file;
      
      file = spc_getenv("PATH_TRANSLATED");
      in   = FileStreamRead(file);
      MemFree(file);
      if (in == NULL)
        rc = (*req->error)(req,HTTP_BADREQ,"","bad request");
      else
      {
        gd.htmldump = in;
        LineSFormat(req->out,"$","Status: %a\r\nContent-type: text/html\r\n\r\n",status);
        generic_cb("main",req->out,NULL);
        rc = 0;
      }
    }
  }
  
  MemFree(status);
  return(rc);
}

/********************************************************************/

static int cmd_cgi_get_edit(Request req)
{
  ddt(req != NULL);
  return((*req->error)(req,HTTP_BADREQ,"","bad request"));
}

/***********************************************************************/

int main_cgi_post(Cgi cgi,int argc,char *argv[])
{
  struct request req;
  int            rc;
  
  ddt(cgi != NULL);
  
  rc = cgi_init(cgi,&req);
  if (rc != ERR_OKAY)
    return((*req.error)(&req,HTTP_ISERVERERR,"","cgi_init() failed"));
  
  req.command = cmd_cgi_post_new;  
  gd.req      = &req;
  
  CgiListMake(cgi);
  
  set_c_updatetype 	(CgiListGetValue(cgi,"updatetype"));
  set_gf_emailupdate	(CgiListGetValue(cgi,"email"));
  set_c_conversion 	(CgiListGetValue(cgi,"filter"));
  set_m_author     	(CgiListGetValue(cgi,"author"),&req);
  set_m_cgi_post_command(CgiListGetValue(cgi,"cmd"),&req);
  
  req.title     = CgiListGetValue(cgi,"title");
  req.class     = CgiListGetValue(cgi,"class");
  req.date      = CgiListGetValue(cgi,"date");
  req.origbody  = CgiListGetValue(cgi,"body");
  req.body      = dup_string(req.origbody);

  if (
       (emptynull_string(req.author))
       || (emptynull_string(req.title))
       || (emptynull_string(req.body))
     )
  {
    return((*req.error)(&req,HTTP_BADREQ,"","errors-missing"));
  }

  if (req.class == NULL)
    req.class = dup_string("");
  
  if (authenticate_author(&req) == FALSE)
  {
    return((*req.error)(&req,HTTP_UNAUTHORIZED,"$ $","errors-author not authenticated got [%a] wanted [%b]",req.author,CgiListGetValue(cgi,"author")));
  }

  rc = (*req.command)(&req);
  return(rc);  
}

/************************************************************************/

static void set_m_cgi_post_command(char *value,Request req)
{
  ddt(req != NULL);

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
  ddt(req != NULL);
  
  if (emptynull_string(value))
    req->author = get_remote_user();
  else
    req->author = dup_string(value);
    
  req->origauthor = dup_string(req->author);
}

/************************************************************************/

static int cmd_cgi_post_new(Request req)
{
  int rc;
  
  ddt(req != NULL);
  
  rc = entry_add(req);
  if (rc == ERR_OKAY)
  {
    generate_pages(req);
    if (cf_weblogcom)   notify_weblogcom();
    if (gf_emailupdate) notify_emaillist();
    D(ddtlog(ddtstream,"$","about to redirect to %a",c_fullbaseurl);)
    LineSFormat(
    	req->out,
	"i $",
	"Status: %a\r\n"
	"Location: %b\r\n"
	"Content-type: text/html\r\n"
	"\r\n"
	"<HTML>\n"
	"  <HEAD><TITLE>Go here</TITLE></HEAD>\n"
	"  <BODY><A HREF='%b'>Go here</A></BODY>\n"
	"</HTML>\n",
	HTTP_MOVETEMP,
	c_fullbaseurl
    );
  }
  else
    rc = (*req->error)(req,HTTP_ISERVERERR,"","couldn't add entry");
  
  return(rc);
}

/***********************************************************************/

static int cmd_cgi_post_show(Request req)
{
  List      listofdays;
  BlogDay   day;
  BlogEntry entry;
  struct tm date;

  ddt(req != NULL);
  
  gd.f.edit = TRUE;
  
  ListInit(&listofdays);
  
  if (emptynull_string(req->date))
  {
    time_t     t;
    struct tm *now;
    
    t    = time(NULL);
    now  = localtime(&t);
    date = *now;
  }
  else
  {
    char *p;
    date.tm_sec   =  0;
    date.tm_min   =  0;
    date.tm_hour  =  1;
    date.tm_wday  =  0;
    date.tm_yday  =  0;
    date.tm_isdst = -1;
    date.tm_year  = strtoul(req->date,&p,10); p++;
    date.tm_mon   = strtoul(p,&p,10); p++;
    date.tm_mday  = strtoul(p,NULL,10);
    tm_to_tm(&date);
  }
  
  fix_entry(req);
  
  BlogDayRead(&day,&gd.now);
  BlogEntryNew(&entry,req->title,req->class,req->author,req->body,strlen(req->body));
  BlogDayEntryAdd(day,entry);
  
  ListAddTail(&listofdays,&day->node);
  
  day->stentry = day->endentry;
  
  LineS(req->out,"Status: 200\r\nContent-type: text/html\r\n\r\n");
  
  generic_cb("main",req->out,&listofdays);
  
  BlogDayFree(&day);
  
  return(0);  
}

/**********************************************************************/

static int cmd_cgi_post_edit(Request req)
{
  return((req->error)(req,HTTP_BADREQ,"","bad request"));
}

/***********************************************************************/

static int cgi_error(Request req,int level,char *format,char *msg, ... )
{
  Stream in;
  char *file;

  {
    char   *docroot;
    Stream  stmp;

    docroot = spc_getenv("DOCUMENT_ROOT");
    stmp    = StringStreamWrite();
    LineSFormat(stmp,"$ i","%a/errors/%b.html",docroot,level);
    file    = StringFromStream(stmp);
    StreamFree(stmp);
    MemFree(docroot);
  }

  in = FileStreamRead(file);
  if (in == NULL)
  {
    LineSFormat(
  	req->out,
  	"i",
	"Status: %a\r\n"
        "Content-type: text/html\r\n"
        "\r\n"
        "<html>\n"
        "<head>\n"
        "<title>Error %a</title>\n"
        "</head>\n"
        "<body>\n"
        "<h1>Error %a</h1>\n"
        "</body>\n"
        "</html>\n"
        "\n",
        level
         );
  }
  else
  {
    gd.htmldump = in;
    LineSFormat(
    	req->out,
    	"i",
    	"Status: %a\r\n"
    	"Content-type: text/html\r\n"
    	"\r\n",
    	level);
    generic_cb("main",req->out,NULL);
  }

  StreamFree(in);
  return(ERR_OKAY);
}

/**********************************************************************/

static int cgi_init(Cgi cgi,Request req)
{
  char *script;
  int   rc;
  
  ddt(cgi != NULL);
  ddt(req != NULL);
  
  memset(req,0,sizeof(struct request));
  
  req->error = cgi_error;
  req->in    = StdinStream;
  req->out   = StdoutStream;
  req->cgi   = cgi;
  
  script = CgiEnvGet(cgi,"SCRIPT_FILENAME");
  if (!empty_string(script))
  {
    rc = GlobalsInit(script);
    if (rc != ERR_OKAY)
      return(rc);
  }
  
  BlogInit();
  BlogDatesInit();
  return(ERR_OKAY);
}

/***********************************************************************/

