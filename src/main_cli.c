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

#include <cgilib7/conf.h>
#include <cgilib7/rfc822.h>
#include <cgilib7/util.h>

#include "backend.h"
#include "frontend.h"
#include "blogutil.h"
#include "conversion.h"
#include "main.h"

typedef int (*clicmd__f)(Blog *,Request *);

/*******************************************************************/

static int mailfile_readdata(Blog *blog,Request *req)
{
  FILE   *output;
  List    headers;
  size_t  size;
  
  assert(blog != NULL);
  assert(req  != NULL);
  
  ListInit(&headers);
  RFC822HeadersRead(stdin,&headers);
  
  req->author     = safe_strdup(PairListGetValue(&headers,"AUTHOR"));
  req->title      = safe_strdup(PairListGetValue(&headers,"TITLE"));
  req->class      = safe_strdup(PairListGetValue(&headers,"CLASS"));
  req->status     = safe_strdup(PairListGetValue(&headers,"STATUS"));
  req->date       = safe_strdup(PairListGetValue(&headers,"DATE"));
  req->adtag      = safe_strdup(PairListGetValue(&headers,"ADTAG"));
  req->conversion = TO_conversion(PairListGetValue(&headers,"FILTER"),blog->config.conversion);
  req->f.email    = TO_email(PairListGetValue(&headers,"EMAIL"),blog->config.email.notify);
  
  PairListFree(&headers);       /* got everything we need, dump this */
  
  if (authenticate_author(blog,req) == false)
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

static int mail_setup_data(Blog *blog,Request *req)
{
  List    headers;
  char   *line = NULL;
  size_t  size = 0;
  
  assert(blog != NULL);
  assert(req  != NULL);
  
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
  
  return mailfile_readdata(blog,req);
}

/*******************************************************************/

static int cli_error(Blog *blog,Request *request,int level,char const *msg, ... )
{
  va_list args;
  
  (void)blog;
  (void)request;
  assert(level >= 0);
  assert(msg   != NULL);
  
  fprintf(stderr,"Error %d: ",level);
  
  va_start(args,msg);
  vfprintf(stderr,msg,args);
  va_end(args);
  
  fputc('\n',stderr);
  
  return 1;
}

/*************************************************************************/

static int cmd_cli_new(Blog *blog,Request *req)
{
  int rc;
  
  assert(blog != NULL);
  assert(req  != NULL);
  
  if (req->f.emailin)
    rc = mail_setup_data(blog,req);
  else
    rc = mailfile_readdata(blog,req);
    
  if (rc != 0)
  {
    fprintf(stderr,"Cannot process new entry\n");
    return EXIT_FAILURE;
  }
  
  if (entry_add(blog,req))
  {
    generate_pages(blog,req);
    return 0;
  }
  else
  {
    fprintf(stderr,"Failed to create new entry\n");
    return EXIT_FAILURE;
  }
}

/****************************************************************************/

static int cmd_cli_show(Blog *blog,Request *req)
{
  int rc = -1;
  
  assert(blog != NULL);
  assert(req  != NULL);
  
  if (req->f.regenerate)
    rc = generate_pages(blog,req);
  else if (req->f.today)
    rc = generate_thisday(blog,req,stdout,blog->now);
  else if (req->f.thisday)
  {
    if (!thisday_new(&req->tumbler,req->reqtumbler))
      rc = cli_error(blog,req,HTTP_BADREQ,"bad request");
    else if (req->tumbler.redirect)
      rc = cli_error(blog,req,HTTP_MOVEPERM,"Redirect: %02d/%02d",req->tumbler.start.month,req->tumbler.start.day);
    else
      rc = generate_thisday(blog,req,stdout,req->tumbler.start);
  }
  else
  {
    if (req->reqtumbler == NULL)
    {
      template__t template;
      
      template.template = blog->config.templates[0].template;
      template.items    = blog->config.templates[0].items;
      template.pagegen  = "days";
      template.reverse  = true;
      template.fullurl  = false;
      
      rc = pagegen_days(blog,req,&template,stdout);
    }
    else
    {
      if (tumbler_new(&req->tumbler,req->reqtumbler,&blog->first,&blog->last))
      {
        if (req->tumbler.redirect)
        {
          char *tum = tumbler_canonical(&req->tumbler);
          rc = cli_error(blog,req,HTTP_MOVEPERM,"Redirect: %s",tum);
          free(tum);
          return rc;
        }
        rc = tumbler_page(blog,req,&req->tumbler,cli_error);
      }
      else
        rc = cli_error(blog,req,HTTP_NOTFOUND,"tumbler error---nothing found");
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
  Blog      *blog;
  Request    request;
  int        rc;
  
  request_init(&request);
  
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
             return cli_error(NULL,NULL,HTTP_ISERVERERR,"%s: %s",optarg,strerror(errno));
           break;
      case OPT_EMAIL:
           request.f.emailin = true;
           break;
      case OPT_ENTRY:
           request.reqtumbler = optarg;
           break;
      case OPT_REGENERATE:
           request.f.regenerate = true;
           break;
      case OPT_FORCENOTIFY:
           forcenotify = true;
           break;
      case OPT_TODAY:
           request.f.today = true;
           break;
      case OPT_THISDAY:
           request.f.thisday  = true;
           request.reqtumbler = optarg;
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
  
  blog = BlogNew(config);
  if (blog == NULL)
    return cli_error(NULL,NULL,HTTP_ISERVERERR,"%s: failed to initialize",config ? config : "missing config file");
    
  if (forcenotify)
  {
    if (!blog->config.email.notify)
    {
      fprintf(stderr,"No email notifiation list\n");
      rc = EXIT_FAILURE;
      goto cleanup_return;
    }
    
    BlogEntry *entry = BlogEntryRead(blog,&blog->last);
    
    if (entry != NULL)
    {
      notify_emaillist(entry);
      BlogEntryFree(entry);
      rc = EXIT_SUCCESS;
      goto cleanup_return;
    }
    else
    {
      fprintf(stderr,"Cannot force notify\n");
      rc = EXIT_FAILURE;
      goto cleanup_return;
    }
  }
  
  rc = (*command)(blog,&request);
cleanup_return:
  BlogFree(blog);
  request_free(&request);
  return rc;
}

/************************************************************************/
