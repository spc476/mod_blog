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

#define _GNU_SOURCE 1

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>
#include <assert.h>

#include <syslog.h>
#include <unistd.h>

#include <cgilib6/rfc822.h>
#include <cgilib6/util.h>
#include <cgilib6/pair.h>

#include "conf.h"
#include "system.h"
#include "conversion.h"
#include "frontend.h"
#include "fix.h"
#include "timeutil.h"
#include "blog.h"

/***********************************************************/

void	set_c_updatetype	(char *const);
void	set_gf_emailupdate	(char *const);
void	set_c_conversion	(char *const);

/************************************************************/

const char    *c_name;
const char    *c_basedir;
const char    *c_webdir;
const char    *c_baseurl;
const char    *c_fullbaseurl;
const char    *c_htmltemplates;
const char    *c_tabtemplates;
const char    *c_rsstemplates;
const char    *c_atomtemplates;
const char    *c_tabfile;
bool           cf_tabreverse  = true;
const char    *c_daypage;
int            c_days         = -1;
const char    *c_rssfile;
const char    *c_atomfile;
int            c_rssitems     = 15;
bool           cf_rssreverse  = true;
const char    *c_author;
const char    *c_email;
const char    *c_authorfile;
const char    *c_updatetype   = "NewEntry";
const char    *c_lockfile     = "/tmp/.mod_blog.lock";
char          *c_emaildb;
const char    *c_emailsubject;
const char    *c_emailmsg;
int            c_tzhour       = -5;	/* Eastern */
int            c_tzmin        =  0;
const char    *c_overview;
void	     (*c_conversion)(FILE *,FILE *) =  html_conversion;
bool           cf_facebook    = false;
const char    *c_facebook_ap_id;
const char    *c_facebook_ap_secret;
const char    *c_facebook_user;

const char    *g_templates;
bool           gf_emailupdate = true;
volatile bool  gf_debug       = false;
Blog           g_blog;
struct display gd =
{
  { false , false , false , false , true , true , false , false} ,
  INDEX,
  "programming",	/* default tag for advertising */
  NULL
};

/****************************************************/

int GlobalsInit(const char *conf)
{
  FILE        *input;
  List         headers;
  struct pair *ppair;
  
  ListInit(&headers);
  
  if (conf == NULL)
  {
    conf = getenv("REDIRECT_BLOG_CONFIG");
    if (conf == NULL)
    {
      conf = getenv("BLOG_CONFIG");
      if (conf == NULL)
      {
        syslog(LOG_ERR,"env BLOG_CONFIG not defined");
        return ERR_ERR;
      }
    }
  }
  
  input = fopen(conf,"r");
  if (input == NULL)
  {
    syslog(LOG_ERR,"%s: %s",conf,strerror(errno));
    return(ERR_ERR);
  }
  
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
      c_basedir = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"WEBDIR") == 0)
    {
      c_webdir = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"BASEURL") == 0)
    {
      char *p = strrchr(ppair->value,'/');
      if (p != NULL) *p = '\0';
      c_baseurl = strdup(ppair->value);
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
      c_fullbaseurl = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"TEMPLATES") == 0)
    {
      c_htmltemplates = strdup(ppair->value);
      g_templates    = c_htmltemplates;
    }
    else if (strcmp(ppair->name,"DAYPAGE") == 0)
    {
      c_daypage = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"DAYS") == 0)
    {
      c_days = strtoul(ppair->value,NULL,10);
    }
    else if (strcmp(ppair->name,"RSSFILE") == 0)
    {
      c_rssfile = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"RSSTEMPLATES") == 0)
    {
      c_rsstemplates = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"RSSFIRST") == 0)
    {
      down_string(ppair->value);
      if (strcmp(ppair->value,"earliest") == 0)
        cf_rssreverse = false;
      else if (strcmp(ppair->value,"latest") == 0)
        cf_rssreverse = true;
      else
        cf_rssreverse = false;
    }
    else if (strcmp(ppair->name,"ATOMTEMPLATES") == 0)
    {
      c_atomtemplates = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"ATOMFILE") == 0)
    {
      c_atomfile = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"TABFILE") == 0)
    {
      c_tabfile = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"TABTEMPLATES") == 0)
    {
      c_tabtemplates = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"TABFIRST") == 0)
    {
      down_string(ppair->value);
      if (strcmp(ppair->value,"earliest") == 0)
        cf_tabreverse = false;
      else if (strcmp(ppair->value,"latest") == 0)
        cf_tabreverse = true;
      else
        cf_tabreverse = false;
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
      c_author = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"AUTHORS") == 0)
    {
      c_authorfile = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"LOCKFILE") == 0)
    {
      c_lockfile = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"EMAIL-UPDATE") == 0)
    {
      set_gf_emailupdate(ppair->value);
    }
    else if (strcmp(ppair->name,"EMAIL") == 0)
    {
      c_email = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"NAME") == 0)
    {
      c_name = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"EMAIL-LIST") == 0)
    {
      c_emaildb = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"EMAIL-SUBJECT") == 0)
    {
      c_emailsubject = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"EMAIL-MESSAGE") == 0)
    {
      c_emailmsg = strdup(ppair->value);
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
      gd.adtag = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"OVERVIEW") == 0)
    {
      c_overview    = strdup(ppair->value);
      gd.f.overview = true;
    }
    else if (strcmp(ppair->name,"DEBUG") == 0)
    {
      if (strcmp(ppair->value,"true") == 0)
        gf_debug = true;
    }
    else if (strcmp(ppair->name,"FACEBOOK-AP-ID") == 0)
    {
      c_facebook_ap_id = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"FACEBOOK-AP-SECRET") == 0)
    {
      c_facebook_ap_secret = strdup(ppair->value);
    }
    else if (strcmp(ppair->name,"FACEBOOK-USER") == 0)
    {
      c_facebook_user = strdup(ppair->value);
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
  fclose(input);
  
  /*-----------------------------------------------------
  ; derive the setting of c_facebook from the given data
  ;------------------------------------------------------*/
  
  cf_facebook =    (c_facebook_ap_id     != NULL)
                && (c_facebook_ap_secret != NULL)
                && (c_facebook_user      != NULL);

  g_blog = BlogNew(c_basedir,c_lockfile);
  if (g_blog == NULL)
    return(ERR_ERR);

  /*-------------------------------------------------------
  ; for most sorting routines, I just assume C sorting
  ; conventions---this makes sure I have those for sorting
  ; and searching only.
  ;--------------------------------------------------------*/
  
  setlocale(LC_COLLATE,"C");

  return(ERR_OKAY);
}

/********************************************************************/

void set_c_updatetype(char *const value)
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

void set_gf_emailupdate(char *const value)
{
  if (value && !empty_string(value))
  {
    up_string(value);
    if (strcmp(value,"NO") == 0)
      gf_emailupdate = false;
    else if (strcmp(value,"YES") == 0)
      gf_emailupdate = true;
  }
}

/***************************************************************************/

void set_c_conversion(char *const value)
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

void set_cf_facebook(char *const value)
{
  if (!emptynull_string(value))
  {
    up_string(value);
    if (strcmp(value,"NO") == 0)
      cf_facebook = false;
    else if (strcmp(value,"YES") == 0)
    {
      if (
              (c_facebook_ap_id     != NULL)
           && (c_facebook_ap_secret != NULL)
           && (c_facebook_user      != NULL)
          )
        cf_facebook = true;
    }
  }
}

/*************************************************************************/

