
/***************************************************
*
* Copyright 2001 by Sean Conner.  All Rights Reserved.
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
*****************************************************/

#define GLOBALS

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <cgi/ddt.h>
#include <cgi/memory.h>
#include <cgi/buffer.h>
#include <cgi/util.h>
#include <cgi/pair.h>

#include "conf.h"
#include "system.h"
#include "conversion.h"
#include "frontend.h"
#include "fix.h"

/***********************************************************/

void	set_g_updatetype	(char *);
void	set_g_emailupdate	(char *);
void	set_g_conversion	(char *);

/************************************************************/

#ifdef PARALLEL_HACK
  const char *g_scriptname;
#endif

const char    *g_name;
const char    *g_basedir;
const char    *g_webdir;
const char    *g_baseurl;
const char    *g_fullbaseurl;
const char    *g_templates;
const char    *g_rsstemplates;
const char    *g_tabtemplates;
const char    *g_tabfile;
int            g_tabreverse   = TRUE;
const char    *g_daypage;
int            g_days         = -1;
const char    *g_rssfile;
int            g_rssitems     = 15;
int            g_rssreverse   = TRUE;
int            g_styear       = -1;
int            g_stmon        = -1;
int            g_stday        = -1;
const char    *g_author;
const char    *g_email;
const char    *g_authorfile;
const char    *g_updatetype   = "NewEntry";
const char    *g_lockfile;
int            g_weblogcom    = FALSE;
const char    *g_weblogcomurl = "http://newhome.weblogs.com/pingSiteForm";
int            g_emailupdate  = FALSE;
char          *g_emaildb;
const char    *g_emailsubject;
const char    *g_emailmsg;
int            g_tzhour       = -5;	/* Eastern */
int            g_tzmin        =  0;
const char    *g_backend;
void	     (*g_conversion)(char *,Buffer,Buffer)  =  html_conversion;
volatile int   g_debug        = FALSE;

#ifdef NEW
  struct display gd =
  {
    m_callbacks,
    CALLBACKS,
    { FALSE , FALSE , FALSE , FALSE , TRUE , TRUE } ,
    INDEX,
    NULL
  };
#endif

/****************************************************/

int GlobalsInit(char *fspec)
{
  char   *cfs;
  char   *ext;
  int     rc;
  Buffer  input;
  Buffer  linput;
  
  ddt(fspec != NULL);

#ifdef PARALLEL_HACK  
  g_scriptname = dup_string(fspec);
#endif

  cfs = dup_string(fspec);
  ext = strstr(cfs,".cnf");

  if (ext == NULL)
  {
    ext = strstr(cfs,".cgi");
    if (ext == NULL)
      return(ErrorPush(AppErr,GLOBALINIT,APPERR_CONF,"$",fspec));
    ext[2] = 'n';
    ext[3] = 'f';
  }

  rc = FileBuffer(&input,cfs,MODE_READ);
  if (rc != ERR_OKAY)
    return(ErrorPush(AppErr,GLOBALINIT,APPERR_CONFOPEN,"$",cfs));

  rc = LineBuffer(&linput,input);
  if (rc != ERR_OKAY)
    return(ErrorPush(AppErr,GLOBALINIT,APPERR_CONFOPEN,"$",cfs));
    
  while(TRUE)
  {
    char         buffer[BUFSIZ];
    size_t       size;
    char        *pt;
    struct pair *ppair;

    size = sizeof(buffer);
    LineRead822(linput,buffer,&size);
    if ((size == 0) || (empty_string(buffer))) break;
    
    pt           = buffer;
    ppair        = PairNew(&pt,':','\0');
    ppair->name  = up_string(trim_space(ppair->name));
    if (!empty_string(ppair->value))
      ppair->value = trim_space(ppair->value);
    
    if (strcmp(ppair->name,"BASEDIR") == 0)
    {
      g_basedir = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"WEBDIR") == 0)
    {
      g_webdir = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"BASEURL") == 0)
    {
      char *p = strrchr(ppair->value,'/');
      if (p != NULL) *p = '\0';
      g_baseurl = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"FULLBASEURL") == 0)
    {
      char *p = strrchr(ppair->value,'/');
      if (p != NULL) *p = '\0';
      g_fullbaseurl = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"TEMPLATES") == 0)
    {
      g_templates = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"DAYPAGE") == 0)
    {
      g_daypage = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"DAYS") == 0)
    {
      g_days = strtoul(ppair->value,NULL,10);
    }
    else if (strcmp(ppair->name,"RSSFILE") == 0)
    {
      g_rssfile = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"RSSTEMPLATES") == 0)
    {
      g_rsstemplates = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"RSSFIRST") == 0)
    {
      down_string(ppair->value);
      if (strcmp(ppair->value,"earliest") == 0)
        g_rssreverse = FALSE;
      else if (strcmp(ppair->value,"latest") == 0)
        g_rssreverse = TRUE;
      else
        g_rssreverse = FALSE;
    }
    else if (strcmp(ppair->name,"TABFILE") == 0)
    {
      g_tabfile = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"TABTEMPLATES") == 0)
    {
      g_tabtemplates = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"TABFIRST") == 0)
    {
      down_string(ppair->value);
      if (strcmp(ppair->value,"earliest") == 0)
        g_tabreverse = FALSE;
      else if (strcmp(ppair->value,"latest") == 0)
        g_tabreverse = TRUE;
      else
        g_tabreverse = FALSE;
    }
    else if (strcmp(ppair->name,"STARTDATE") == 0)
    {
      char *p = ppair->value;
      
      g_styear = strtoul(p,&p,10); p++;
      g_stmon  = strtoul(p,&p,10); p++;
      g_stday  = strtoul(p,NULL,10);
    }
    else if (strcmp(ppair->name,"AUTHOR") == 0)
    {
      g_author = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"AUTHORS") == 0)
    {
      g_authorfile = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"LOCKFILE") == 0)
    {
      g_lockfile = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"EMAIL") == 0)
    {
      g_email = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"NAME") == 0)
    {
      g_name = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"WEBLOGCOM") == 0)
    {
      down_string(ppair->value);
     
      if (strcmp(ppair->value,"true") == 0)
        g_weblogcom = TRUE;
      else if (strcmp(ppair->value,"false") == 0)
        g_weblogcom = FALSE;
    }
    else if (strcmp(ppair->name,"EMAIL-LIST") == 0)
    {
      g_emailupdate = TRUE;
      g_emaildb     = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"EMAIL-SUBJECT") == 0)
    {
      g_emailsubject = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"EMAIL-MESSAGE") == 0)
    {
      g_emailmsg = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"TIMEZONE") == 0)
    {
      char *p = ppair->value;
      
      g_tzhour = strtoul(p,&p,10);
      p++;	/* skip `:' */
      g_tzmin  = strtoul(p,NULL,10);
    }
    else if (strcmp(ppair->name,"BACKEND") == 0)
    {
      g_backend = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"CONVERSION") == 0)
    {
      set_g_conversion(ppair->value);
    }
    else if (strcmp(ppair->name,"DEBUG") == 0)
    {
      if (strcmp(ppair->value,"true") == 0)
        g_debug = TRUE;
    }
    else if (strncmp(ppair->name,"_SYSTEM",7) == 0)
    {
      SystemLimit(ppair);
    }
    else if (strcmp(ppair->name,"COMMENT") == 0)
    {
      /* just here to ensure comments are available */
    }
    
    PairFree(ppair);
  }
  
  BufferFree(&linput);
  BufferFree(&input);
  MemFree(cfs,strlen(cfs) + 1);
  return(ERR_OKAY);
}

/********************************************************************/

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
    else if (strcmp(value,"YES") == 0)
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

/**************************************************************************/
