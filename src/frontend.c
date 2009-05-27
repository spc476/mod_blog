
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
  OPT_HELP,
  OPT_DEBUG
};

/*************************************************************************/

static const struct option coptions[] =
{
  { "config"	, required_argument	, NULL	, OPT_CONFIG } ,
  { "cmd"	, required_argument	, NULL	, OPT_CMD    } ,
  { "file"	, required_argument	, NULL	, OPT_FILE   } ,
  { "email"	, no_argument		, NULL	, OPT_EMAIL  } ,
  { "update"	, required_argument	, NULL	, OPT_UPDATE } ,
  { "entry"	, required_argument	, NULL	, OPT_ENTRY  } ,
  { "stdin"     , no_argument           , NULL	, OPT_STDIN  } ,
  { "help"	, no_argument		, NULL	, OPT_HELP   } ,
  { "debug"	, no_argument		, NULL  , OPT_DEBUG  } ,
  { NULL        , 0			, NULL	, 0          }
};

/*************************************************************************/

int main(int argc,char *argv[])
{
  Cgi cgi;
  int rc;
  
  MemInit   ();
  BufferInit();
  DdtInit   ();
  CleanInit ();
  
  if (CgiNew(&cgi,NULL) == ERR_OKAY)
  {
    switch(CgiMethod(cgi))
    {
      case GET:  rc = main_cgi_get (cgi,argc,argv); break;
      case POST: rc = main_cgi_post(cgi,argc,argv); break;
      default:   rc = EXIT_FAILURE;
    }
  }
  else
    rc = main_cli(argc,argv);
  
  return(rc);
}

/************************************************************************/

int main_cli(int argc,char *argv[])
{
  int   isemail  = FALSE;
  int   isupdate = FALSE;
  char *tumbler  = NULL;
  char *config   = NULL;
  int   rc;
  
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
           rc = FileBuffer(
      case OPT_EMAIL:
           isemail = TRUE;
           break;
      case OPT_UPDATE:
           isupdate = TRUE;
           set_g_updatetype(optarg);
           break;
      case OPT_ENTRY:
           tumbler = dupstring(optarg);
           break;
      case OPT_DEBUG:
           g_debug = TRUE;
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
  
  if (isupdate)
  {
  }
  
  if (tumbler != NULL)
  {
    Tumbler spec;
    
    rc = TublerNew(&spec,&optarg);
    if (rc == ERR_OKAY)
      rc = tumbler_page(stdout,spec);
    else
    {
      fprintf(stderr,"tumbler error---nothing found\n");
      rc = EXIT_FAILURE;
    }
    return(rc);
  }
  
  rc = generate_pages();
    
  return(rc);
}

/************************************************************************/

int main_cgi_get(Cgi cgi,int argc,char *argv[])
{
}

/***********************************************************************/

int main_cgi_post(Cgi cgi,int argc,char *argv[])
{
  CgiListMake(cgi);
  
  set_g_updatetype (CgiListGetValue(cgi,"updatetype"));
  set_g_emailupdate(CgiListGetValue(cgi,"email"));
  set_g_conversion (CgiListGetValue(cgi,"filter"));
  set_m_author     (CgiListGetValue(cgi,"author"));
  
  m_title   = CgiListGetValue(cgi,"title");
  m_class   = CgiListGetValue(cgi,"class");
  m_date    = CgiListGetValue(cgi,"date");
  m_cgibody = CgiListGetValue(cgi,"body");
  m_cmd     = CgiListGetValue(cgi,"cmd");

  if (
  	(m_author == NULL)
  	|| (empty_string(m_author))
  	|| (m_title == NULL)
  	|| (empty_string(m_title))
  	|| (m_class == NULL)
  	|| (empty_string(m_class))
  	|| (m_cgibody == NULL)
  	|| (empty_string(m_cgibody))
  )
  {
    do_error(cgi,"errors-missing");
    return(EXIT_FAILURE);
  }
  
  if (authenticate_author() == FALSE)
  {
    do_error(cgi,"errors-author not authenticated");
    return(EXIT_FAILURE);
  }
  
    
}

/************************************************************************/

void set_m_author(char *value)
{
  if ((value == NULL) || (empty_string(value)))
    value = spc_getenv("REMOTE_USER");
  
  m_author = dupstring(value);
}

/**********************************************************************/
  
void set_g_updatetype(char *value)
{
  if (value == NULL) return;
  if (empty_string(value)) return;
  
  up_string(value);
  
  if (strcmp(value,"NEW") == 0)
    g_updatetype = "NewEntry";
  else if (strcmp(value,"MODIFY") == 0)
    g_updatetype = "ModifiedEntry";
  else if (strcmp(value,"EDIT") == 0)
    g_updatetype = "ModifiedEntry";
  else if (strcmp(value,"TEMPLATE") == 0)
    g_updatetype = "TemplateChange";
  else 
    g_updatetype = "Other";
}

/************************************************************************/

void set_g_emailupdate(char *value)
{
  if (value && !empty_string(value))
  {
    up_string(value);
    if (strcmp(value,"NO") == 0)
      g_emailupdate = FALSE;
    else if (strcmp(value,"YES" == 0)
      g_emailupdate = TRUE;
  }
}

/***************************************************************************/

void set_g_conversion(char *value)
{
  if (value == NULL) return;
  if (empty_string(value)) return;
  up_string(value);
  
  if (strcmp(value,"TEXT") == 0)
    g_conversion = text_conversion;
  else if (strcmp(value,"MIXED") == 0)
    g_conversion = mixed_conversion;
  else if (strcmp(value,"HTML") == 0)
    g_conversion = html_conversion;
}

/***************************************************************************/

