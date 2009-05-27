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

#include <getopt.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/stream.h>
#include <cgilib/pair.h>
#include <cgilib/errors.h>
#include <cgilib/htmltok.h>
#include <cgilib/rfc822.h>
#include <cgilib/http.h>
#include <cgilib/cgi.h>
#include <cgilib/util.h>

#include "conf.h"
#include "globals.h"
#include "blog.h"
#include "frontend.h"
#include "timeutil.h"
#include "wbtum.h"
#include "fix.h"

enum
{
  OPT_NONE,
  OPT_CONFIG,
  OPT_CMD,
  OPT_FILE,
  OPT_EMAIL,
  OPT_UPDATE,
  OPT_ENTRY,
  OPT_STDIN,
  OPT_REGENERATE,
  OPT_FORCENOTIFY,
  OPT_HELP,
  OPT_DEBUG
};

/*******************************************************************/

static int	cmd_cli_new		(Request);
static int	cmd_cli_show		(Request);
static void	get_cli_command		(Request,char *);
static int	mail_setup_data		(Request);
static int	mailfile_readdata	(Request);
static void	collect_body		(Stream,Stream);
static int	cli_error		(Request,int,char *,char *, ... );

/*************************************************************************/

static const struct option coptions[] =
{
  { "config"	   , required_argument	, NULL	, OPT_CONFIG 	  } ,
  { "regenerate"   , no_argument	, NULL	, OPT_REGENERATE  } ,
  { "regen"	   , no_argument	, NULL	, OPT_REGENERATE  } ,
  { "cmd"	   , required_argument	, NULL	, OPT_CMD    	  } ,
  { "file"	   , required_argument	, NULL	, OPT_FILE   	  } ,
  { "email"	   , no_argument	, NULL	, OPT_EMAIL  	  } ,
  { "update"	   , required_argument	, NULL	, OPT_UPDATE 	  } ,
  { "entry"	   , required_argument	, NULL	, OPT_ENTRY  	  } ,
  { "stdin"        , no_argument        , NULL	, OPT_STDIN  	  } ,
  { "force-notify" , no_argument 	, NULL  , OPT_FORCENOTIFY } ,
  { "help"	   , no_argument	, NULL	, OPT_HELP   	  } ,
  { "debug"	   , no_argument	, NULL  , OPT_DEBUG  	  } ,
  { NULL           , 0			, NULL	, 0          	  }
};

/*************************************************************************/

int main_cli(int argc,char *argv[])
{
  struct request  req;
  char           *config = NULL;
  int             rc;
  int             forcenotify = FALSE;
  
  memset(&req,0,sizeof(struct request));

  req.command    = cmd_cli_show;
  req.error      = cli_error;
  req.in         = StdinStream;
  req.out        = StdoutStream;
  gd.req         = &req;
  
  while(1)
  {
    int option = 0;
    int c;
    
    c = getopt_long_only(argc,argv,"",coptions,&option);
    if (c == EOF) 
      break;
    else switch(c)
    {
      case OPT_CONFIG:
           config = optarg;
           break;
      case OPT_FILE:
           req.in = FileStreamRead(optarg);
           req.f.filein = TRUE;
           break;
      case OPT_EMAIL:
           req.f.emailin = TRUE;
           break;
      case OPT_STDIN:
           req.f.std_in = TRUE;
           break;
      case OPT_UPDATE:
           req.f.update = TRUE;
           set_c_updatetype(optarg);
           break;
      case OPT_ENTRY:
           req.reqtumbler = dup_string(optarg);
           break;
      case OPT_DEBUG:
           gf_debug = TRUE;
           break;
      case OPT_REGENERATE:
           req.f.regenerate = TRUE;
           break;
      case OPT_FORCENOTIFY:
           forcenotify = TRUE;
	   break;
      case OPT_CMD:
           get_cli_command(&req,optarg);
           break;
      case OPT_HELP:
      default:
           LineSFormat(
	   	StderrStream,
		"$",
		"usage: %a --options... \n"
		"\t--config file\n"
		"\t--regenerate | --regen\n"
		"\t--cmd ('new' | 'show')\n"
		"\t--file file\n"
		"\t--email\n"
		"\t--update ('new' | 'modify' | 'template' | 'other')\n"
		"\t--entry <tumbler>\n"
		"\t--stdin\n"
		"\t--force-notify\n"
		"\t--help\n"
		"\t--debug\n",
		argv[0]
	      );
	    return(EXIT_FAILURE);
    }
  }

  if (config == NULL)
    return((*req.error)(&req,HTTP_ISERVERERR,"","no configuration file specified"));
  
  rc = GlobalsInit(config);
  if (rc != ERR_OKAY)
    return((*req.error)(&req,HTTP_ISERVERERR,"","could not open cofiguration file %s",config));
  
  rc = BlogInit();
  if (rc != ERR_OKAY)
    return((*req.error)(&req,HTTP_ISERVERERR,"","could not initialize blog"));

  rc = BlogDatesInit();
  if (rc != ERR_OKAY)
    return((*req.error)(&req,HTTP_ISERVERERR,"","could not initialize dates"));
  
  if (forcenotify)
  {
    notify_emaillist();
    return(0);
  }

  rc = (*req.command)(&req);
  return(rc);  
}

/************************************************************************/

static int cmd_cli_new(Request req)
{
  int rc;
  
  ddt(req         != NULL);
  ddt(req->f.cgiin == FALSE);

  /*-------------------------------------------------
  ; req.f.stdin and req.f.filein may be uneeded
  ;-------------------------------------------------*/
  
  if (req->f.emailin)
    rc = mail_setup_data(req);
  else
    rc = mailfile_readdata(req);
  
  if (rc != ERR_OKAY)
    return(EXIT_FAILURE);

  rc = entry_add(req);
  if (rc == ERR_OKAY)
  {
    generate_pages(req);  
    if (cf_weblogcom)   notify_weblogcom();
    if (gf_emailupdate) notify_emaillist();
  }
  
  return(rc);
}

/****************************************************************************/

static int cmd_cli_show(Request req)
{
  int rc;
  
  ddt(req            != NULL);
  ddt(req->f.emailin == FALSE);
  ddt(req->f.filein  == FALSE);
  ddt(req->f.update  == FALSE);

  if (req->f.regenerate)
    rc = generate_pages(req);  
  else
  {
    if (req->reqtumbler == NULL)
      rc = primary_page(
      		req->out,
		gd.now.tm_year,
		gd.now.tm_mon,
		gd.now.tm_mday
	);
    else
    {
      rc = TumblerNew(&req->tumbler,&req->reqtumbler);
      if (rc == ERR_OKAY)
        rc = tumbler_page(req->out,req->tumbler);
      else
        rc = (*req->error)(req,HTTP_NOTFOUND,"","tumbler error---nothing found");
    }
  }

  return(rc);
}

/********************************************************************/

static void get_cli_command(Request req,char *value)
{
  ddt(req != NULL);

  if (emptynull_string(value)) return;
  up_string(value);
  
  if (strcmp(value,"NEW") == 0)
    req->command = cmd_cli_new;
  else if (strcmp(value,"SHOW") == 0)
    req->command = cmd_cli_show;
  else if (strcmp(value,"PREVIEW") == 0)
    req->command = cmd_cli_show;
}

/*************************************************************************/

static int mail_setup_data(Request req)
{
  List  headers;
  char *line;
  
  ddt(req            != NULL);
  ddt(req->f.emailin == TRUE);

  ListInit(&headers);
  
  line = LineSRead(req->in);		/* skip Unix 'From ' line */
  MemFree(line);  
  
  /*----------------------------------------------
  ; skip the header section for now---just ignore it
  ;-------------------------------------------------*/

  RFC822HeadersRead(req->in,&headers);
  PairListFree(&headers);

  return(mailfile_readdata(req));
}
  
/*******************************************************************/

static int mailfile_readdata(Request req)
{
  Stream  output;
  List    headers;
  char   *email;
  char   *filter;

  ddt(req     != NULL);
  ddt(req->in != NULL);

  ListInit(&headers);
  RFC822HeadersRead(req->in,&headers);

  req->author = PairListGetValue(&headers,"AUTHOR");
  req->title  = PairListGetValue(&headers,"TITLE");
  req->class  = PairListGetValue(&headers,"CLASS");
  req->date   = PairListGetValue(&headers,"DATE");
  email       = PairListGetValue(&headers,"EMAIL");
  filter      = PairListGetValue(&headers,"FILTER");
  
  if (req->author != NULL) req->author = dup_string(req->author);

  if (req->title  != NULL) 
    req->title  = dup_string(req->title);
  else
    req->title = dup_string("");

  if (req->class  != NULL) 
    req->class  = dup_string(req->class);
  else
    req->class = dup_string("");

  if (req->date   != NULL) req->date = dup_string(req->date);
  if (email       != NULL) set_gf_emailupdate(email);
  if (filter      != NULL) set_c_conversion(filter);

  PairListFree(&headers);	/* got everything we need, dump this */
  
  if (authenticate_author(req) == FALSE)
  {
    StreamFree(req->in);
    return(ERR_ERR);
  }
  
  output = StringStreamWrite();
  collect_body(output,req->in);
  req->origbody = StringFromStream(output);
  req->body     = dup_string(req->origbody);
  StreamFree(output);
  return(ERR_OKAY);
}

/***************************************************************************/

static void collect_body(Stream output,Stream input)
{
  HtmlToken token;
  int       rc;
  int       t;
  
  rc = HtmlParseNew(&token,input);
  if (rc != ERR_OKAY)
    return;
  
  while((t = HtmlParseNext(token)) != T_EOF)
  {
    if (t == T_TAG)
    {
      if (strcmp(HtmlParseValue(token),"/HTML") == 0) break;
      if (strcmp(HtmlParseValue(token),"/BODY") == 0) break;
      HtmlParsePrintTag(token,output);
    }
    else if (t == T_STRING)
      LineS(output,HtmlParseValue(token));
    else if (t == T_COMMENT)
      LineSFormat(output,"$","<!%a>",HtmlParseValue(token));
  }

  HtmlParseFree(&token);
}

/***********************************************************************/

static int cli_error(Request req,int level,char *format,char *msg, ... )
{
  va_list args;
  
  va_start(args,msg);
  LineSFormat(StderrStream,"i3.3","Error %a: ",level);
  LineSFormatv(StderrStream,format,msg,args);
  StreamWrite(StderrStream,'\n');
  va_end(args);
  return(ERR_ERR);
}

/**************************************************************************/

