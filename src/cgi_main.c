
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <cgi/memory.h>
#include <cgi/buffer.h>
#include <cgi/errors.h>
#include <cgi/ddt.h>
#include <cgi/clean.h>
#include <cgi/cgi.h>
#include <cgi/util.h>

#include "conf.h"
#include "globals.h"
#include "blog.h"
#include "frontend.h"
#include "timeutil.h"
#include "wbtum.h"
#include "fix.h"

static int	cgi_init		(Cgi,Request);
void		set_m_cgi_post_command	(char *,Request);
void		set_m_author		(char *,Request);
int		cmd_cgi_post_new	(Request);
int		cmd_cgi_post_show	(Request);
int		cgi_error		(Request,char *, ... );

/************************************************************************/

static int cgi_init(Cgi cgi,Request req)
{
  char *script;
  int   rc;
  
  memset(req,0,sizeof(struct request));
  
  req->error = cgi_error;
  req->fpin  = NULL;
  req->fpout = stdout;
  req->bin   = NULL;
  req->bout  = CgiBufferOut(cgi);
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

/*************************************************************************/

int main_cgi_get(Cgi cgi,int argc,char *argv[])
{
  struct request  req;
  int             rc;
  
  ddt(cgi         != NULL);
  ddt(req         != NULL);
  ddt(req.f.cgiin == TRUE);

  rc = cgi_init(cgi,&req);
  if (rc != ERR_OKAY)
    return((*req.error)(&req,"cgi_init() failed"));

  req.reqtumbler = CgiEnvGet(cgi,"PATH_INFO");

  if ((empty_string(req.reqtumbler)) || (strcmp(req.reqtumbler,"/") == 0))
    rc = generate_pages(&req);
  else
  {
    req.reqtumbler++;
    rc = TumblerNew(&req.tumbler,&req.reqtumbler);
    if (rc == ERR_OKAY)
      rc = tumbler_page(req.fpout,req.tumbler);
    else
      ;
      /* return 400 error */
  }

  return(rc);
}

/***********************************************************************/

int main_cgi_post(Cgi cgi,int argc,char *argv[])
{
  struct request req;
  int            rc;
  
  rc = cgi_init(cgi,&req);
  if (rc != ERR_OKAY)
    return((*req.error)(&req,"cgi_init() failed"));
  
  req.command = cmd_cgi_post_new;  
  CgiListMake(cgi);
  
  set_g_updatetype 	(CgiListGetValue(cgi,"updatetype"));
  set_g_emailupdate	(CgiListGetValue(cgi,"email"));
  set_g_conversion 	(CgiListGetValue(cgi,"filter"));
  set_m_author     	(CgiListGetValue(cgi,"author"),&req);
  set_m_cgi_post_command(CgiListGetValue(cgi,"cmd"),&req);
  
  req.title = CgiListGetValue(cgi,"title");
  req.class = CgiListGetValue(cgi,"class");
  req.date  = CgiListGetValue(cgi,"date");
  req.body  = CgiListGetValue(cgi,"body");

  if (
  	(req.author == NULL)
  	|| (empty_string(req.author))
  	|| (req.title == NULL)
  	|| (empty_string(req.title))
  	|| (req.class == NULL)
  	|| (empty_string(req.class))
  	|| (req.body == NULL)
  	|| (empty_string(req.body))
  )
  {
    /* return 400 error */
    return((*req.error)(&req,"errors-missing"));
  }
  
  if (authenticate_author(&req) == FALSE)
  {
    /* return 400 error */
    return((*req.error)(&req,"errors-author not authenticated"));
  }

  rc = (*req.command)(&req);
  return(rc);  
}

/************************************************************************/

void set_m_cgi_post_command(char *value,Request req)
{
  ddt(req != NULL);
  
  if (value == NULL) return;
  if (empty_string(value)) return;
  up_string(value);
  
  if (strcmp(value,"NEW") == 0)
    req->command = cmd_cgi_post_new;
  else if (strcmp(value,"SHOW") == 0)
    req->command = cmd_cgi_post_show;
}

/************************************************************************/

void set_m_author(char *value,Request req)
{
  if ((value == NULL) || (empty_string(value)))
    value = spc_getenv("REMOTE_USER");
  
  req->author = dup_string(value);
}

/************************************************************************/

int cmd_cgi_post_new(Request req)
{
  int rc;
  
  rc = entry_add(req);
  if (rc == ERR_OKAY)
    ;
    /* XXX - redirect to entry just made ... */
  else
    /* return 500 error */
    (*req->error)(req,"couldn't add entry");
  
  return(rc);
}

/***********************************************************************/

int cmd_cgi_post_show(Request req)
{
  return(ERR_ERROR);
}

/**********************************************************************/

int cgi_error(Request req,char *msg, ... )
{
  return(ERR_OKAY);
}

/**********************************************************************/

