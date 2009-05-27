
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include <cgi/memory.h>
#include <cgi/buffer.h>
#include <cgi/ddt.h>
#include <cgi/clean.h>
#include <cgi/errors.h>
#include <cgi/cgi.h>
#include <cgi/util.h>

#include "conf.h"
#include "globals.h"
#include "blog.h"
#include "frontend.h"
#include "timeutil.h"
#include "wbtum.h"
#include "fix.h"

#define MAILSETUPDATA	(ERR_APP + 1)
#define SETUPERR_INPUT	(ERR_APP + 2)
#define FILESETUPDATA	(ERR_APP + 3)
#define SETUPERR_AUTH	(ERR_APP + 4)
#define SETUPERR_MEMORY	(ERR_APP + 5)

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
  OPT_HELP,
  OPT_DEBUG
};

/*******************************************************************/

int		cmd_cli_new		(Request);
int		cmd_cli_show		(Request);
void		get_cli_command		(Request,char *);
static int	mail_setup_data		(Request);
static int	file_setup_data		(Request);
static int	mailfile_readdata	(Request);
int		cli_error		(Request,char *, ... );

/*************************************************************************/

static const struct option coptions[] =
{
  { "config"	 , required_argument	, NULL	, OPT_CONFIG 	 } ,
  { "regenerate" , no_argument		, NULL	, OPT_REGENERATE } ,
  { "regen"	 , no_argument		, NULL	, OPT_REGENERATE } ,
  { "cmd"	 , required_argument	, NULL	, OPT_CMD    	 } ,
  { "file"	 , required_argument	, NULL	, OPT_FILE   	 } ,
  { "email"	 , no_argument		, NULL	, OPT_EMAIL  	 } ,
  { "update"	 , required_argument	, NULL	, OPT_UPDATE 	 } ,
  { "entry"	 , required_argument	, NULL	, OPT_ENTRY  	 } ,
  { "stdin"      , no_argument          , NULL	, OPT_STDIN  	 } ,
  { "help"	 , no_argument		, NULL	, OPT_HELP   	 } ,
  { "debug"	 , no_argument		, NULL  , OPT_DEBUG  	 } ,
  { NULL         , 0			, NULL	, 0          	 }
};

/*************************************************************************/

int main_cli(int argc,char *argv[])
{
  struct request  req;
  char           *config;
  int             rc;
  
  memset(&req,0,sizeof(struct request));

  req.command    = cmd_cli_show;
  req.error      = cli_error;
  req.fpin       = stdin;
  req.fpout      = stdout;

  StdinBuffer (&req.bin);
  StdoutBuffer(&req.bout);
  
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
           set_g_updatetype(optarg);
           break;
      case OPT_ENTRY:
           req.reqtumbler = dup_string(optarg);
           break;
      case OPT_DEBUG:
           g_debug = TRUE;
           break;
      case OPT_REGENERATE:
           req.f.regenerate = TRUE;
           break;
      case OPT_CMD:
           get_cli_command(&req,optarg);
           break;
      case OPT_HELP:
      default:
           return ((*req.error)(&req,"%s - unknown option or help",argv[0]));
    }
  }

  if (config == NULL)
    return((*req.error)(&req,"no configuration file specified"));
  
  rc = GlobalsInit(config);
  if (rc != ERR_OKAY)
  {
    ErrorLog();
    return((*req.error)(&req,"could not open cofiguration file %s",config));
  }
  
  BlogInit();
  BlogDatesInit();
  
  rc = (*req.command)(&req);
  return(rc);  
}

/************************************************************************/

int cmd_cli_new(Request req)
{
  int rc;
  
  ddt(req         != NULL);
  ddt(req.f.cgiin == FALSE);

  /*-------------------------------------------------
  ; req.f.stdin and req.f.filein may be uneeded
  ;-------------------------------------------------*/
  
  if (req->f.emailin)
    rc = mail_setup_data(req);
  else
    rc = file_setup_data(req);
  
  if (rc != ERR_OKAY)
  {
    ErrorLog();
    ErrorClear();
    return(EXIT_FAILURE);
  }

  rc = entry_add(req);

  if (rc == ERR_OKAY)
  {
    generate_pages(req);  
    if (g_weblogcom)   notify_weblogcom();
    if (g_emailupdate) notify_emaillist();
  }
  
  return(rc);
}

/****************************************************************************/

int entry_add(Request req)
{
  struct tm date;
  BlogDay   day;
  BlogEntry entry;
  int       lock;
  
  ddt(req != NULL);
    
  if ((req->date == NULL) || (empty_string(req->date)))
  {
    time_t t;
    struct tm *now;
    
    t = time(NULL);
    now = localtime(&t);
    date = *now;
  }
  else
  {
    char *p;
    
    date.tm_sec   = 0;
    date.tm_min   = 0;
    date.tm_hour  = 1;
    date.tm_wday  = 0;
    date.tm_yday  = 0;
    date.tm_isdst = -1;
    date.tm_year  = strtoul(req->date,&p,10); p++;
    date.tm_mon   = strtoul(p,&p,10); p++;
    date.tm_mday  = strtoul(p,NULL,10);
    tm_to_tm(&date);
  }
  
  fix_entry(req);

  if (g_authorfile) lock = BlogLock(g_lockfile);
  BlogDayRead(&day,&date);
  BlogEntryNew(&entry,req->title,req->class,req->author,req->body,strlen(req->body));
  BlogDayEntryAdd(day,entry);
  BlogDayWrite(day);
  BlogDayFree(&day);
  if (g_authorfile) BlogUnlock(lock);
  
  return(ERR_OKAY);
}

/************************************************************************/

int cmd_cli_show(Request req)
{
  int rc;
  
  ddt(req           != NULL);
  ddt(req.f.stdin   == FALSE);
  ddt(req.f.emailin == FALSE);
  ddt(req.f.filein  == FALSE);
  ddt(req.f.update  == FALSE);

  if (req->f.regenerate)
    rc = generate_pages(req);  
  else
  {
    if (req->reqtumbler == NULL)
    {
      /* ??? */
    }
    else
    {
      rc = TumblerNew(&req->tumbler,&req->reqtumbler);
      if (rc == ERR_OKAY)
        rc = tumbler_page(req->fpout,req->tumbler);
      else
        rc = (*req->error)(req,"tumbler error---nothing found");
    }
  }

  return(rc);
}

/********************************************************************/

void get_cli_command(Request req,char *value)
{
  ddt(req != NULL);

  if (value == NULL) return;
  if (empty_string(value)) return;
  up_string(value);
  
  if (strcmp(value,"NEW") == 0)
    req->command = cmd_cli_new;
  else if (strcmp(value,"SHOW") == 0)
    req->command = cmd_cli_show;
}

/*************************************************************************/

static int mail_setup_data(Request req)
{
  int    rc;
  size_t size;
  char   data[BUFSIZ];
  
  ddt(req != NULL);
  ddt(req.f.emailin == TRUE);
  
  rc = LineBuffer(&req->lbin,req->bin);
  if (rc != ERR_OKAY)
    return(ErrorPush(AppErr,MAILSETUPDATA,SETUPERR_INPUT,""));
  
  size = BUFSIZ;
  LineRead(req->lbin,data,&size);	/* skip Unix 'From ' line */
  
  /*----------------------------------------------
  ; skip the header section for now---just ignore it
  ;-------------------------------------------------*/
  
  do
  {
    size = BUFSIZ;
    LineRead822(req->lbin,data,&size);
  } while ((size != 0) && (!empty_string(data)));
  
  return(mailfile_readdata(req));
}

/*******************************************************************/

static int file_setup_data(Request req)
{
  int rc;
  
  rc = LineBuffer(&req->lbin,req->bin);
  if (rc != ERR_OKAY)
    return(ErrorPush(AppErr,FILESETUPDATA,SETUPERR_INPUT,""));
  
  return(mailfile_readdata(req));
}

/*******************************************************************/

static int mailfile_readdata(Request req)
{
  char         data       [BUFSIZ];
  char         megabuffer [65536UL];
  char        *pt;
  struct pair *ppair;
  size_t       size;
  Buffer       output;
  int          rc;

  ddt(req      != NULL);
  ddt(req.lbin != NULL);
  
  memset(megabuffer,0,sizeof(megabuffer));
 
  /*--------------------------------------------
  ; now read the user supplied header
  ;--------------------------------------------*/

  while(TRUE)
  {
    size = BUFSIZ;
    LineRead822(req->lbin,data,&size);

    /*------------------------------------
    ; break this up, it might not be working
    ; correctly ... 
    ;------------------------------------*/

    if (size == 0) break;
    if (empty_string(data)) break;

    pt           = data;
    ppair        = PairNew(&pt,':','\0');
    ppair->name  = up_string(trim_space(ppair->name));
    ppair->value = trim_space(ppair->value);

    if (strcmp(ppair->name,"AUTHOR") == 0)
      req->author = dup_string(ppair->value);
    else if (strcmp(ppair->name,"TITLE") == 0)
      req->title = dup_string(ppair->value);
    else if (strcmp(ppair->name,"SUBJECT") == 0)
      req->title = dup_string(ppair->value);
    else if (strcmp(ppair->name,"CLASS") == 0)
      req->class = dup_string(ppair->value);
    else if (strcmp(ppair->name,"DATE") == 0)
      req->date = dup_string(ppair->value);
    else if (strcmp(ppair->name,"EMAIL") == 0)
      set_g_emailupdate(ppair->value);
    else if (strcmp(ppair->name,"FILTER") == 0)
      set_g_conversion(ppair->value);      
    PairFree(ppair);
  }

  if (authenticate_author(req) == FALSE)
  {
    BufferFree(&req->lbin);
    BufferFree(&req->bin);
    return(ErrorPush(AppErr,MAILSETUPDATA,SETUPERR_AUTH,""));
  }

  rc = MemoryBuffer(&output,megabuffer,sizeof(megabuffer));
  if (rc != ERR_OKAY)
    return(ErrorPush(AppErr,MAILSETUPDATA,SETUPERR_MEMORY,""));

  collect_body(output,req->lbin);
  BufferFree(&output);
  req->body = dup_string(megabuffer);

  return(ERR_OKAY);
}

/***************************************************************************/

int cli_error(Request req,char *msg, ... )
{
  return(ERR_OKAY);
}

/**************************************************************************/

