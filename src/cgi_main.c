
#include <stddef.h>
#include <stdlib.h>

#include <cgi/memory.h>
#include <cgi/buffer.h>
#include <cgi/ddt.h>
#include <cgi/clean.h>
#include <cgi/cgi.h>

/*************************************************************************/

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

void get_cgi_command(Request req,char *value)
{
  ddt(req != NULL);
  
  if (value == NULL) return;
  if (empty_string(value)) return;
  up_string(value);
  
  if (strcmp(value,"NEW") == 0)
    req->command = cmd_cgi_post_new;
  else if (strcmp(value,"EDIT") == 0)
    req->command = cmd_cgi_post_edit;
  else if (strcmp(value,"DELETE") == 0)
    req->command = cmd_cgi_post_delete;
  else if (strcmp(value,"SHOW") == 0)
    req->command = cmd_cgi_post_show;
}

/************************************************************************/

void set_m_author(char *value)
{
  if ((value == NULL) || (empty_string(value)))
    value = spc_getenv("REMOTE_USER");
  
  m_author = dupstring(value);
}

/************************************************************************/

