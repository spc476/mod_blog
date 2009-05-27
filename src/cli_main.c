
#include <stddef.h>
#include <stdlib.h>

#include <getopt.h>

#include <cgi/memory.h>
#include <cgi/buffer.h>
#include <cgi/ddt.h>
#include <cgi/clean.h>
#include <cgi/cgi.h>

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

/*************************************************************************/

static const struct option coptions[] =
{
  { "config"	 , required_argument	, NULL	, OPT_CONFIG 	 } ,
  { "regenerate" , no_argument		, NULL	, OPT_REGENERATE } ,
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
  struct request req;
  int            rc;
  
  memset(&req,0,sizeof(struct request));

  req.command    = cmd_cli_show;
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
           rc = FileBuffer(
      case OPT_EMAIL:
           req.f.emailin = TRUE;
           break;
      case OPT_STDIN:
           req.f.stdin = TRUE;
           break;
      case OPT_UPDATE:
           req.f.update = TRUE;
           set_g_updatetype(optarg);
           break;
      case OPT_ENTRY:
           req.tumbler = dupstring(optarg);
           break;
      case OPT_DEBUG:
           g_debug = TRUE;
           break;
      case OPT_REGENERATE:
           req.f.regen = TRUE;
           break;
      case OPT_CMD:
           get_cli_command(&req,optarg);
           break;
      case OPT_HELP:
      default:
           return (usage(argv[0],"unknown option or help"));
    }
  }

  if (config == NULL)
  {
    fprintf(stderr,"no configuation file specified\n");
    return(EXIT_FAILURE);
  }
  
  rc = GlobalsInit(config);
  if (rc != ERR_OKAY)
  {
    ErrorLog();
    fprintf(stderr,"could not open configuration file %s\n",config);
    return(rc);
  }
  
  BlogInit();
  BlogDatesInit();
  
  rc = (*req.command)(&req);
  return(rc);  
}

/************************************************************************/

int cmd_cli_new(Request req)
{
  struct tm date;
  BlogDay   day;
  BlogEntry entry;
  int       lock;
  int       rc;
  
  int rc;
  ddt(req         != NULL);
  ddt(req.f.cgiin == FALSE);

  /*-------------------------------------------------
  ; req.f.stdin and req.f.filein may be uneeded
  ;-------------------------------------------------*/
  
  if (req.f.emailin)
    rc = mail_setup_data(req);
  else
    rc = file_setup_data(req);
  
  if (rc != ERR_OKAY)
  {
    ErrorLog();
    ErrorClear();
    return(EXIT_FAILURE);
  }
  
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
  BlogEntryNew(&entry,req->title,req->author,req->body,strlen(req->body));
  BlogDayEntryAdd(day,entry);
  BlogDayWrite(day);
  BlogDayFree(&day);
  if (g_authorfile) BlogUnlock(lock);
  
  /* XXX --- add the calls to what we used to shell out for */

  generate_pages(req);
  
  if (g_weblogcom)   notify_weblogcom();
  if (g_emailupdate) notify_emaillist();
  
  return(EXIT_SUCCESS);
}

/************************************************************************/

int cmd_cli_show(Request req)
{
  int rc;
  
  ddt(req           != NULL);
  ddt(req.f.stdin   == FALSE);
  ddt(req.f.emailin == FALSE);
  ddt(req.f.filein  == FALSE);
  ddt(req.f.cgiin   == FALSE);
  ddt(req.f.update  == FALSE);

  if (req.f.regen)
    rc = generate_pages(req);  
  else
  {
    if (req.tumbler == NULL)
      /* ??? */
    else
    {
      rc = TublerNew(&req->tumbler,req->reqtumbler);
      if (rc == ERR_OKAY)
        rc = tumbler_page(req);
      else
      {
        fprintf(stderr,"tumbler error---nothing found\n");
        rc = EXIT_FAILURE;
      }
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
  
  return(mailfile_readdata());
}

/*******************************************************************/

static int file_setup_data(Request req)
{
  rc = LineBuffer(&req->lbin,req->bin);
  if (rc != ERR_OKAY)
    return(ErrorPush(AppErr,FILESETUPDATA,SETUPERR_INPUT,""));
  
  return(mailfile_readdata(req));
}

/*******************************************************************/

static int mailfile_readdata(Request req)
{
  ddt(req      != NULL);
  ddt(req.lbin != NULL);
  
  char         data       [BUFSIZ];
  char         megabuffer [65536UL];
  char        *pt;
  struct pair *ppair;
  size_t       size;
  Buffer       output;
  int          rc;

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

  if (authenticate_author() == FALSE)
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

