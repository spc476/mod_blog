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
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#include <getopt.h>
#include <lua.h>
#include <gdbm.h>

#include <cgilib6/conf.h>
#include <cgilib6/pair.h>
#include <cgilib6/htmltok.h>
#include <cgilib6/rfc822.h>
#include <cgilib6/cgi.h>
#include <cgilib6/util.h>

#include "conf.h"
#include "blog.h"
#include "frontend.h"
#include "timeutil.h"
#include "wbtum.h"
#include "fix.h"
#include "backend.h"
#include "globals.h"

enum
{
  OPT_NONE,
  OPT_CONFIG,
  OPT_CMD,
  OPT_FILE,
  OPT_EMAIL,
  OPT_UPDATE,
  OPT_ENTRY,
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
static int	cli_error		(Request,int,char *, ... );

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
  int             forcenotify = false;
  
  memset(&req,0,sizeof(struct request));

  req.command    = cmd_cli_show;
  req.error      = cli_error;
  req.in         = stdin;
  req.out        = stdout;
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
           req.in = fopen(optarg,"r");
           break;
      case OPT_EMAIL:
           req.f.emailin = true;
           break;
      case OPT_UPDATE:
           req.f.update = true;
           set_c_updatetype(optarg);
           break;
      case OPT_ENTRY:
           req.reqtumbler = optarg;
           break;
      case OPT_DEBUG:
           gf_debug = true;
           break;
      case OPT_REGENERATE:
           req.f.regenerate = true;
           break;
      case OPT_FORCENOTIFY:
           forcenotify = true;
	   break;
      case OPT_CMD:
           get_cli_command(&req,optarg);
           break;
      case OPT_HELP:
      default:
           fprintf(
           	stderr,
		"usage: %s --options... \n"
		"\t--config file\n"
		"\t--regenerate | --regen\n"
		"\t--cmd ('new' | 'show' *)\n"
		"\t--file file\n"
		"\t--email\n"
		"\t--update ('new' * | 'modify' | 'template' | 'other')\n"
		"\t--entry <tumbler>\n"
		"\t--force-notify\n"
		"\t--help\n"
		"\t--debug\n"
		"\n"
		"\tVersion: " GENERATOR "\n"
		"\t\t%s\n"
		"\t\t%s\n"
		"\t\t%s\n"
		"\t* default value\n"
		"",
		argv[0],
		cgilib_version,
		LUA_RELEASE,
		gdbm_version
	      );
	   gd.req = NULL;
	   return(EXIT_FAILURE);
    }
  }

  rc = GlobalsInit(config);
  if (rc != 0)
  {
    gd.req = NULL;
    return((*req.error)(&req,HTTP_ISERVERERR,"could not open cofiguration file %s or could not initialize dates",config));
  }
  
  if (forcenotify)
  {
    notify_emaillist();
    gd.req = NULL;
    return(0);
  }

  rc = (*req.command)(&req);
  
  if (req.in != stdin) fclose(req.in);
  
  free(req.update);
  free(req.origauthor);
  free(req.author);
  free(req.name);
  free(req.title);
  free(req.class);
  free(req.status);
  free(req.date);
  free(req.origbody);
  free(req.body);
  gd.req = NULL;
  return(rc);  
}

/************************************************************************/

static int cmd_cli_new(Request req)
{
  int rc;
  
  assert(req != NULL);

  if (req->f.emailin)
    rc = mail_setup_data(req);
  else
    rc = mailfile_readdata(req);
  
  if (rc != 0)
    return(EXIT_FAILURE);

  rc = entry_add(req);
  if (rc == 0)
  {
    if (cf_emailupdate) notify_emaillist();
    generate_pages(req);  
  }
  
  return(rc);
}

/****************************************************************************/

static int cmd_cli_show(Request req)
{
  int rc;
  
  assert(req            != NULL);
  assert(req->f.emailin == false);
  assert(req->f.update  == false);

  if (req->f.regenerate)
    rc = generate_pages(req);  
  else
  {
    if (req->reqtumbler == NULL)
    {
      template__t template;
      
      template.template = (char *)c_htmltemplates;
      template.items    = c_days;
      template.pagegen  = pagegen_days;
      template.reverse  = true;
      template.fullurl  = false;
      
      rc = pagegen_days(&template,req->out,g_blog);
    }
    else
    {
      rc = TumblerNew(&req->tumbler,&req->reqtumbler);
      if (rc == 0)
      {
        if (req->tumbler->flags.redirect)
        {
          char *tum = TumblerCanonical(req->tumbler);
          rc = (*req->error)(req,HTTP_MOVEPERM,"Redirect: %s",tum);
          free(tum);
          return(rc);
        }
        rc = tumbler_page(req->out,req->tumbler);
        TumblerFree(&req->tumbler);
      }
      else
        rc = (*req->error)(req,HTTP_NOTFOUND,"tumbler error---nothing found");
    }
  }

  return(rc);
}

/********************************************************************/

static void get_cli_command(Request req,char *value)
{
  assert(req != NULL);

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
  List    headers;
  char   *line;
  size_t  size;
  
  assert(req            != NULL);
  assert(req->f.emailin == true);

  line = NULL;
  size = 0;
  
  ListInit(&headers);
  getline(&line,&size,req->in);	/* skip Unix 'From ' line */
  free(line);  
  
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
  FILE   *output;
  List    headers;
  char   *email;
  char   *filter;
  size_t  size;

  assert(req     != NULL);
  assert(req->in != NULL);

  ListInit(&headers);
  RFC822HeadersRead(req->in,&headers);

  req->author = PairListGetValue(&headers,"AUTHOR");
  req->title  = PairListGetValue(&headers,"TITLE");
  req->class  = PairListGetValue(&headers,"CLASS");
  req->status = PairListGetValue(&headers,"STATUS");
  req->date   = PairListGetValue(&headers,"DATE");
  email       = PairListGetValue(&headers,"EMAIL");
  filter      = PairListGetValue(&headers,"FILTER");
  
  if (req->author != NULL) req->author = strdup(req->author);

  if (req->title  != NULL) 
    req->title  = strdup(req->title);
  else
    req->title = strdup("");

  if (req->class  != NULL) 
    req->class  = strdup(req->class);
  else
    req->class = strdup("");

  if (req->status != NULL)
    req->status = strdup(req->status);
  else
    req->status = strdup("");
  
  if (req->date   != NULL) req->date = strdup(req->date);
  if (email       != NULL) set_cf_emailupdate(email);
  if (filter      != NULL) set_c_conversion(filter);
  
  PairListFree(&headers);	/* got everything we need, dump this */
  
  if (authenticate_author(req) == false)
  {
    fclose(req->in);
    return(EPERM);
  }
  
  output = open_memstream(&req->origbody,&size);
  fcopy(output,req->in);
  fclose(output);

  req->body     = strdup(req->origbody);
  return(0);
}

/***************************************************************************/

static int cli_error(Request req __attribute__((unused)),int level,char *msg, ... )
{
  va_list args;
  
  fprintf(stderr,"Error %d: ",level);
  
  va_start(args,msg);
  vfprintf(stderr,msg,args);
  va_end(args);

  fputc('\n',stderr);
  
  return -1;
}

/**************************************************************************/

