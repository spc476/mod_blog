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
#include <getopt.h>
#include <gdbm.h>

#include <cgilib6/conf.h>
#include <cgilib6/rfc822.h>
#include <cgilib6/util.h>

#include "backend.h"
#include "frontend.h"
#include "blogutil.h"
#include "conversion.h"
#include "globals.h"

typedef int (*clicmd__f)(Request *);

/*******************************************************************/

static int       cmd_cli_new       (Request *);
static int       cmd_cli_show      (Request *);
static clicmd__f get_cli_command   (char const *);
static int       mail_setup_data   (Request *);
static int       mailfile_readdata (Request *);
static int       cli_error         (int,char const *, ... );

/*************************************************************************/

int main_cli(int argc,char *argv[])
{
  enum
  {
    OPT_NONE,
    OPT_CONFIG,
    OPT_CMD,
    OPT_FILE,
    OPT_EMAIL,
    OPT_ENTRY,
    OPT_REGENERATE,
    OPT_FORCENOTIFY,
    OPT_TODAY,
    OPT_THISDAY,
    OPT_HELP,
  };
  
  static struct option const coptions[] =
  {
    { "config"       , required_argument  , NULL  , OPT_CONFIG      } ,
    { "regenerate"   , no_argument        , NULL  , OPT_REGENERATE  } ,
    { "regen"        , no_argument        , NULL  , OPT_REGENERATE  } ,
    { "cmd"          , required_argument  , NULL  , OPT_CMD         } ,
    { "file"         , required_argument  , NULL  , OPT_FILE        } ,
    { "email"        , no_argument        , NULL  , OPT_EMAIL       } ,
    { "entry"        , required_argument  , NULL  , OPT_ENTRY       } ,
    { "force-notify" , no_argument        , NULL  , OPT_FORCENOTIFY } ,
    { "today"        , no_argument        , NULL  , OPT_TODAY       } ,
    { "thisday"      , required_argument  , NULL  , OPT_THISDAY     } ,
    { "help"         , no_argument        , NULL  , OPT_HELP        } ,
    { NULL           , 0                  , NULL  , 0               }
  };
  
  char      *config      = NULL;
  bool       forcenotify = false;
  clicmd__f  command     = cmd_cli_show;
  
  while(true)
  {
    int   option = 0;
    int   c;
    
    c = getopt_long_only(argc,argv,"",coptions,&option);
    if (c == EOF)
      break;
    else switch(c)
    {
      case OPT_CONFIG:
           config = optarg;
           break;
      case OPT_FILE:
           if (freopen(optarg,"r",stdin) == NULL)
             return cli_error(HTTP_ISERVERERR,"%s: %s",optarg,strerror(errno));
           break;
      case OPT_EMAIL:
           g_request.f.emailin = true;
           break;
      case OPT_ENTRY:
           g_request.reqtumbler = optarg;
           break;
      case OPT_REGENERATE:
           g_request.f.regenerate = true;
           break;
      case OPT_FORCENOTIFY:
           forcenotify = true;
           break;
      case OPT_TODAY:
           g_request.f.today = true;
           break;
      case OPT_THISDAY:
           g_request.f.thisday  = true;
           g_request.reqtumbler = optarg;
           break;
      case OPT_CMD:
           command = get_cli_command(optarg);
           break;
      case OPT_HELP:
      default:
           fprintf(
                stderr,
                "usage: %s --options... \n"
                "\t--config file\n"
                "\t--regenerate | --regen\n"
                "\t--cmd ('new' | 'show' * | 'preview')\n"
                "\t--file file\n"
                "\t--email\n"
                "\t--entry <tumbler>\n"
                "\t--force-notify\n"
                "\t--today\n"
                "\t--thisday <month>/<day>\n"
                "\t--help\n"
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
           return EXIT_FAILURE;
    }
  }
  
  if (!GlobalsInit(config))
    return cli_error(HTTP_ISERVERERR,"%s: failed to initialize",config);
    
  if (forcenotify)
  {
    if (!g_blog->config.email.notify)
    {
      fprintf(stderr,"No email notifiation list\n");
      return EXIT_FAILURE;
    }
    
    BlogEntry *entry = BlogEntryRead(g_blog,&g_blog->last);
    
    if (entry != NULL)
    {
      notify_emaillist(entry);
      BlogEntryFree(entry);
      return EXIT_SUCCESS;
    }
    else
    {
      fprintf(stderr,"Cannot force notify\n");
      return EXIT_FAILURE;
    }
  }
  
  return (command)(&g_request);
}

/************************************************************************/

static int cmd_cli_new(Request *req)
{
  int rc;
  
  assert(req != NULL);
  
  if (req->f.emailin)
    rc = mail_setup_data(req);
  else
    rc = mailfile_readdata(req);
    
  if (rc != 0)
  {
    fprintf(stderr,"Cannot process new entry\n");
    return EXIT_FAILURE;
  }
  
  if (entry_add(req,g_blog))
  {
    generate_pages();
    return 0;
  }
  else
  {
    fprintf(stderr,"Failed to create new entry\n");
    return EXIT_FAILURE;
  }
}

/****************************************************************************/

static int cmd_cli_show(Request *req)
{
  int rc = -1;
  
  assert(req != NULL);
  
  if (req->f.regenerate)
    rc = generate_pages();
  else if (req->f.today)
    rc = generate_thisday(stdout,g_blog->now);
  else if (req->f.thisday)
  {
    if (!thisday_new(&req->tumbler,req->reqtumbler))
      rc = cli_error(HTTP_BADREQ,"bad request");
    else if (req->tumbler.redirect)
      rc = cli_error(HTTP_MOVEPERM,"Redirect: %02d/%02d",req->tumbler.start.month,req->tumbler.start.day);
    else
      rc = generate_thisday(stdout,req->tumbler.start);
  }
  else
  {
    if (req->reqtumbler == NULL)
    {
      template__t template;
      
      template.template = g_blog->config.templates[0].template;
      template.items    = g_blog->config.templates[0].items;
      template.pagegen  = "days";
      template.reverse  = true;
      template.fullurl  = false;
      
      rc = pagegen_days(&template,stdout,g_blog);
    }
    else
    {
      if (tumbler_new(&req->tumbler,req->reqtumbler,&g_blog->first,&g_blog->last))
      {
        if (req->tumbler.redirect)
        {
          char *tum = tumbler_canonical(&req->tumbler);
          rc = cli_error(HTTP_MOVEPERM,"Redirect: %s",tum);
          free(tum);
          return rc;
        }
        rc = tumbler_page(&req->tumbler,cli_error);
      }
      else
        rc = cli_error(HTTP_NOTFOUND,"tumbler error---nothing found");
    }
  }
  
  assert(rc != -1);
  return rc;
}

/********************************************************************/

static clicmd__f get_cli_command(char const *value)
{
  if (emptynull_string(value))
    return cmd_cli_show;
    
  if (strcmp(value,"new") == 0)
    return cmd_cli_new;
  else if (strcmp(value,"show") == 0)
    return cmd_cli_show;
  else if (strcmp(value,"preview") == 0)
    return cmd_cli_show;
  else
  {
    fprintf(stderr,"'%s' not supported, using 'show'\n",value);
    return cmd_cli_show;
  }
}

/*************************************************************************/

static int mail_setup_data(Request *req)
{
  List    headers;
  char   *line = NULL;
  size_t  size = 0;
  
  assert(req != NULL);
  
  ListInit(&headers);
  getline(&line,&size,stdin); /* skip Unix 'From ' line */
  free(line);
  
  /*----------------------------------------------------------------------
  ; In the past, entries have been sent that have been encoded as
  ; "quoted-printable".  I'm not sure the exact conditions that cause it
  ; (recents tests with known non-7bit clean content have been passed
  ; through unencoded) but it is happening now that I've somewhat changed
  ; how I write entries with quoted material.
  ;
  ; For now, check for the existence of the Content-Transfer-Encoding header
  ; and if it's not '7bit', '8bit' or 'binary' (which are identity
  ; encodings) then stop further processing and signal an error.
  ;
  ; At some future point, we might want to decode encoded entries sent via
  ; email.
  ;-----------------------------------------------------------------------*/
  
  RFC822HeadersRead(stdin,&headers);
  
  char *encoding = PairListGetValue(&headers,"CONTENT-TRANSFER-ENCODING");
  if (encoding)
  {
    down_string(encoding);
    if (
            (strcmp(encoding,"7bit")   != 0)
         && (strcmp(encoding,"8bit")   != 0)
         && (strcmp(encoding,"binary") != 0)
       )
    {
      syslog(LOG_ERR,"Content-Transfer-Encoding of '%s' not allowed!",encoding);
      PairListFree(&headers);
      return EINVAL;
    }
  }
  
  PairListFree(&headers);
  
  return mailfile_readdata(req);
}

/*******************************************************************/

static int mailfile_readdata(Request *req)
{
  FILE   *output;
  List    headers;
  size_t  size;
  
  assert(req != NULL);
  
  ListInit(&headers);
  RFC822HeadersRead(stdin,&headers);
  
  req->author     = PairListGetValue(&headers,"AUTHOR");
  req->title      = PairListGetValue(&headers,"TITLE");
  req->class      = PairListGetValue(&headers,"CLASS");
  req->status     = PairListGetValue(&headers,"STATUS");
  req->date       = PairListGetValue(&headers,"DATE");
  req->adtag      = PairListGetValue(&headers,"ADTAG");
  req->conversion = TO_conversion(PairListGetValue(&headers,"FILTER"),g_blog->config.conversion);
  req->f.email    = TO_email(PairListGetValue(&headers,"EMAIL"));
  
  if (req->author != NULL)
    req->author = strdup(req->author);
  else
    req->author = strdup("");
    
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
    
  if (req->adtag != NULL)
    req->adtag = strdup(req->adtag);
  else
    req->adtag = strdup("");
    
  if (req->date != NULL)
    req->date = strdup(req->date);
    
  PairListFree(&headers);       /* got everything we need, dump this */
  
  if (authenticate_author(req,g_blog) == false)
  {
    syslog(LOG_ERR,"'%s' not authorized to post",req->author);
    return EPERM;
  }
  
  output = open_memstream(&req->origbody,&size);
  fcopy(output,stdin);
  fclose(output);
  
  req->body = strdup(req->origbody);
  return 0;
}

/***************************************************************************/

static int cli_error(int level,char const *msg, ... )
{
  va_list args;
  
  assert(level >= 0);
  assert(msg   != NULL);
  
  fprintf(stderr,"Error %d: ",level);
  
  va_start(args,msg);
  vfprintf(stderr,msg,args);
  va_end(args);
  
  fputc('\n',stderr);
  
  return 1;
}

/**************************************************************************/
