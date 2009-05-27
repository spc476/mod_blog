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

#include <cgilib/ddt.h>
#include <cgilib/memory.h>
#include <cgilib/stream.h>
#include <cgilib/rfc822.h>
#include <cgilib/util.h>
#include <cgilib/pair.h>

#include "conf.h"
#include "system.h"
#include "conversion.h"
#include "frontend.h"
#include "fix.h"
#include "timeutil.h"
#include "blog.h"

/***********************************************************/

void	set_c_updatetype	(char *);
void	set_gf_emailupdate	(char *);
void	set_c_conversion	(char *);

/************************************************************/

#ifdef PARALLEL_HACK
  const char *c_scriptname;
#endif

const char    *c_name;
const char    *c_basedir;
const char    *c_webdir;
const char    *c_baseurl;
const char    *c_fullbaseurl;
const char    *c_tabtemplates;
const char    *c_rsstemplates;
const char    *c_atomtemplates;
const char    *c_tabfile;
int            cf_tabreverse  = TRUE;
const char    *c_daypage;
int            c_days         = -1;
const char    *c_rssfile;
const char    *c_atomfile;
int            c_rssitems     = 15;
int            cf_rssreverse  = TRUE;
const char    *c_author;
const char    *c_email;
const char    *c_authorfile;
const char    *c_updatetype   = "NewEntry";
const char    *c_lockfile     = "/tmp/.mod_blog.lock";
int            cf_weblogcom   = FALSE;
const char    *c_weblogcomurl = "http://newhome.weblogs.com/pingSiteForm";
char          *c_emaildb;
const char    *c_emailsubject;
const char    *c_emailmsg;
int            c_tzhour       = -5;	/* Eastern */
int            c_tzmin        =  0;
const char    *c_overview;
void	     (*c_conversion)(char *,Stream,Stream) =  html_conversion;

const char    *g_templates;
int            gf_emailupdate = FALSE;
volatile int   gf_debug       = FALSE;
Blog           g_blog;
struct display gd =
{
  { FALSE , FALSE , FALSE , FALSE , TRUE , TRUE } ,
  INDEX,
  "programming",	/* default tag for advertising */
  NULL
};

/****************************************************/

int GlobalsInit(char *fspec)
{
  Stream       input;
  List         headers;
  char        *cfs;
  char        *ext;
  struct pair *ppair;
  
  ddt(fspec != NULL);

  ListInit(&headers);
  
#ifdef PARALLEL_HACK  
  c_scriptname = dup_string(fspec);
#endif

  cfs = dup_string(fspec);
  ext = strstr(cfs,".cnf");

  if (ext == NULL)
  {
    ext = strstr(cfs,".cgi");
    if (ext == NULL)
      return(ERR_ERR);
    ext[2] = 'n';
    ext[3] = 'f';
  }

  input = FileStreamRead(cfs);
  if (input == NULL)
    return(ERR_ERR);
    
  RFC822HeadersRead(input,&headers);

  for
  (
    ppair = PairListFirst(&headers);
    NodeValid(&ppair->node);
    ppair = (struct pair *)NodeNext(&ppair->node)
  )
  {
    if (strcmp(ppair->name,"BASEDIR") == 0)
    {
      c_basedir = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"WEBDIR") == 0)
    {
      c_webdir = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"BASEURL") == 0)
    {
      char *p = strrchr(ppair->value,'/');
      if (p != NULL) *p = '\0';
      c_baseurl = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"FULLBASEURL") == 0)
    {
      /*--------------------------------------------
      ; FullBaseURL cannot end in a '/' or else bad
      ; things happen.  We don't check for it, and I
      ; forgot why, because it wasn't commented, but 
      ; whatever you do, don't use the commented out
      ; code.
      ;--------------------------------------------*/
#ifdef FIXED_BROKEN_FULLBASEURL
      char *p = strrchr(ppair->value,'/');
      if (p != NULL) *p = '\0';
#endif
      c_fullbaseurl = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"TEMPLATES") == 0)
    {
      g_templates = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"DAYPAGE") == 0)
    {
      c_daypage = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"DAYS") == 0)
    {
      c_days = strtoul(ppair->value,NULL,10);
    }
    else if (strcmp(ppair->name,"RSSFILE") == 0)
    {
      c_rssfile = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"RSSTEMPLATES") == 0)
    {
      c_rsstemplates = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"RSSFIRST") == 0)
    {
      down_string(ppair->value);
      if (strcmp(ppair->value,"earliest") == 0)
        cf_rssreverse = FALSE;
      else if (strcmp(ppair->value,"latest") == 0)
        cf_rssreverse = TRUE;
      else
        cf_rssreverse = FALSE;
    }
    else if (strcmp(ppair->name,"ATOMTEMPLATES") == 0)
    {
      c_atomtemplates = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"ATOMFILE") == 0)
    {
      c_atomfile = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"TABFILE") == 0)
    {
      c_tabfile = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"TABTEMPLATES") == 0)
    {
      c_tabtemplates = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"TABFIRST") == 0)
    {
      down_string(ppair->value);
      if (strcmp(ppair->value,"earliest") == 0)
        cf_tabreverse = FALSE;
      else if (strcmp(ppair->value,"latest") == 0)
        cf_tabreverse = TRUE;
      else
        cf_tabreverse = FALSE;
    }
    else if (strcmp(ppair->name,"STARTDATE") == 0)
    {
      char *p = ppair->value;
      
      gd.begin.year  = strtoul(p,&p,10); p++;
      gd.begin.month = strtoul(p,&p,10); p++;
      gd.begin.day   = strtoul(p,NULL,10);
      gd.begin.part  = 1;      
    }
    else if (strcmp(ppair->name,"AUTHOR") == 0)
    {
      c_author = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"AUTHORS") == 0)
    {
      c_authorfile = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"LOCKFILE") == 0)
    {
      c_lockfile = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"EMAIL") == 0)
    {
      c_email = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"NAME") == 0)
    {
      c_name = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"WEBLOGCOM") == 0)
    {
      down_string(ppair->value);
     
      if (strcmp(ppair->value,"true") == 0)
        cf_weblogcom = TRUE;
      else if (strcmp(ppair->value,"false") == 0)
        cf_weblogcom = FALSE;
    }
    else if (strcmp(ppair->name,"EMAIL-LIST") == 0)
    {
      c_emaildb = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"EMAIL-SUBJECT") == 0)
    {
      c_emailsubject = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"EMAIL-MESSAGE") == 0)
    {
      c_emailmsg = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"TIMEZONE") == 0)
    {
      char *p = ppair->value;
      
      c_tzhour = strtol(p,&p,10);
      p++;	/* skip `:' */
      c_tzmin  = strtoul(p,NULL,10);
    }
    else if (strcmp(ppair->name,"CONVERSION") == 0)
    {
      set_c_conversion(ppair->value);
    }
    else if (strcmp(ppair->name,"ADTAG") == 0)
    {
      gd.adtag = dup_string(ppair->value);
    }
    else if (strcmp(ppair->name,"OVERVIEW") == 0)
    {
      c_overview    = dup_string(ppair->value);
      gd.f.overview = TRUE;
    }
    else if (strcmp(ppair->name,"DEBUG") == 0)
    {
      if (strcmp(ppair->value,"true") == 0)
        gf_debug = TRUE;
    }
    else if (strncmp(ppair->name,"_SYSTEM",7) == 0)
    {
      SystemLimit(ppair);
    }
    else if (strcmp(ppair->name,"COMMENT") == 0)
    {
      /* just here to ensure comments are available */
    }
  }

  PairListFree(&headers);
  StreamFree(input);
  MemFree(cfs);

  g_blog = BlogNew(c_basedir,c_lockfile);
  if (g_blog == NULL)
    return(ERR_ERR);

  return(ERR_OKAY);
}

/********************************************************************/

void set_c_updatetype(char *value)
{
  if (value == NULL) return;
  if (empty_string(value)) return;
  
  up_string(value);
  
  if (strcmp(value,"NEW") == 0)
    c_updatetype = "NewEntry";
  else if (strcmp(value,"MODIFY") == 0)
    c_updatetype = "ModifiedEntry";
  else if (strcmp(value,"EDIT") == 0)
    c_updatetype = "ModifiedEntry";
  else if (strcmp(value,"TEMPLATE") == 0)
    c_updatetype = "TemplateChange";
  else 
    c_updatetype = "Other";
}

/************************************************************************/

void set_gf_emailupdate(char *value)
{
  if (value && !empty_string(value))
  {
    up_string(value);
    if (strcmp(value,"NO") == 0)
      gf_emailupdate = FALSE;
    else if (strcmp(value,"YES") == 0)
      gf_emailupdate = TRUE;
  }
}

/***************************************************************************/

void set_c_conversion(char *value)
{
  if (value == NULL) return;
  if (empty_string(value)) return;
  up_string(value);
  
  if (strcmp(value,"TEXT") == 0)
    c_conversion = text_conversion;
  else if (strcmp(value,"MIXED") == 0)
    c_conversion = mixed_conversion;
  else if (strcmp(value,"HTML") == 0)
    c_conversion = html_conversion;
}

/**************************************************************************/

