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

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <cgilib6/url.h>
#include <cgilib6/util.h>

#include "conf.h"
#include "conversion.h"
#include "frontend.h"
#include "fix.h"
#include "timeutil.h"
#include "blog.h"

/***********************************************************/

void	set_c_updatetype	(char *const);
void	set_gf_emailupdate	(char *const);
void	set_c_conversion	(char *const);
void	set_c_url		(const char *);

static bool	   get_next	(char *,const char **);
static int	   get_field	(lua_State *const restrict,const char *);
static const char *get_string	(lua_State *const restrict,const char *const restrict,const char *const restrict);
static int	   get_int	(lua_State *const restrict,const char *const restrict,const int);
static bool	   get_bool	(lua_State *const restrict,const char *const restrict,const bool);

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
const char    *c_emaildb;
const char    *c_emailsubject;
const char    *c_emailmsg;
int            c_tzhour       = -5;	/* Eastern */
int            c_tzmin        =  0;
const char    *c_overview;
void	     (*c_conversion)(FILE *,FILE *) =  html_conversion;
bool           cf_facebook    = false;	/* set by code */
const char    *c_facebook_ap_id;
const char    *c_facebook_ap_secret;
const char    *c_facebook_user;

lua_State     *g_L;
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
  int rc;
  
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
  
  g_L = luaL_newstate();
  if (g_L == NULL)
  {
    syslog(LOG_ERR,"cannot create Lua state");
    return ERR_ERR;
  }
  
  lua_gc(g_L,LUA_GCSTOP,0);
  luaL_openlibs(g_L);
  lua_gc(g_L,LUA_GCRESTART,0);
  rc = luaL_loadfile(g_L,conf);
  
  if (rc != 0)
  {
    const char *err = lua_tostring(g_L,-1);
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,err);
    return ERR_ERR;
  }
  
  rc = lua_pcall(g_L,0,LUA_MULTRET,0);
  if (rc != 0)
  {
    const char *err = lua_tostring(g_L,-1);
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,err);
    return ERR_ERR;
  }
  
  gf_debug   = get_bool(g_L,"debug",false);
  
  c_name        = get_string(g_L,"name",NULL);
  c_basedir     = get_string(g_L,"basedir",NULL);
  c_webdir      = get_string(g_L,"webdir",NULL);
  gd.adtag      = get_string(g_L,"adtag","programming");
  c_lockfile    = get_string(g_L,"lockfile","/tmp/.mod_blog.lock");
  c_overview    = get_string(g_L,"overview",NULL);
  gd.f.overview = (c_overview != NULL);

  lua_getglobal(g_L,"templates");
  lua_pushinteger(g_L,1);
  lua_gettable(g_L,-2);
  lua_getfield(g_L,-1,"template");
  c_htmltemplates = strdup(lua_tostring(g_L,-1));
  g_templates     = c_htmltemplates;	/* XXX */
  lua_pop(g_L,3);
  
#if 0
  c_htmltemplates = get_string(g_L,"templates.html.template",NULL);
  c_daypage       = get_string(g_L,"templates.html.output",NULL);
  c_days          = get_int   (g_L,"templates.html.days",7);
  g_templates     = c_htmltemplates;	/* XXX --- need to fix */
  
  c_rsstemplates = get_string(g_L,"templates.rss.template",NULL);
  c_rssfile      = get_string(g_L,"templates.rss.output",NULL);
  c_rssitems     = get_int   (g_L,"templates.rss.items",15);
  cf_rssreverse  = get_bool  (g_L,"templates.rss.reverse",true);
  
  c_atomtemplates = get_string(g_L,"templates.atom.template",NULL);
  c_atomfile      = get_string(g_L,"templates.atom.output",NULL);
#endif

  c_author     = get_string(g_L,"author.name",NULL);
  c_email      = get_string(g_L,"author.email",NULL);
  c_authorfile = get_string(g_L,"author.file",NULL);
  
  c_emaildb      = get_string(g_L,"email.list",NULL);
  c_emailsubject = get_string(g_L,"email.subject",NULL);
  c_emailmsg     = get_string(g_L,"email.message",NULL);
  gf_emailupdate = get_bool  (g_L,"email.notify",true);
  
  c_facebook_ap_id     = get_string(g_L,"facebook.ap_id",NULL);
  c_facebook_ap_secret = get_string(g_L,"facebook.ap_secret",NULL);
  c_facebook_user      = get_string(g_L,"facebook.user",NULL);
  
  {
    const char *timezone = get_string(g_L,"timezone","-5:00");
    char       *p;
  
    c_tzhour = strtol(timezone,&p,10);
    p++;
    c_tzmin  = strtoul(p,NULL,10);
    free((void *)timezone);
  }
  
  {
    const char *conversion = get_string(g_L,"conversion","html");
    set_c_conversion((char *)conversion);
    free((void *)conversion);
  }
  
  {
    const char *url = get_string(g_L,"url",NULL);
    set_c_url(url);
    free((void *)url);
  }
  
  {
    const char *d = get_string(g_L,"startdate",NULL);    
    char       *p;
    
    if (d)
    {
      gd.begin.year  = strtoul(d,&p,10); p++;
      gd.begin.month = strtoul(p,&p,10); p++;
      gd.begin.day   = strtoul(p,NULL,10);
      gd.begin.part  = 1;
      free(p);
    }
  }  

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

void set_c_url(const char *turl)
{
  url__t *url;
  size_t  len;
  char   *fbu;
  char   *bu;

  assert(turl != NULL);

  fbu = strdup(turl);
  url = UrlNew(turl);
  bu  = strdup(url->http.path);

  /*-----------------------------------------------------------------------
  ; because of the way link generation happens, both of these *CAN'T* end
  ; with a '/'.  So make sure they don't end with a '/'.
  ;
  ; The reason we go through an intermediate variable is that c_fullbaseurl
  ; and c_baseurl are declared as 'const char *' and we can't modify a
  ; constant memory location.
  ;-----------------------------------------------------------------------*/
  
  len = strlen(fbu);
  if (len) len--;
  if (fbu[len] == '/') fbu[len] = '\0';
  
  len = strlen(bu);
  if (len) len--;
  if (bu[len] == '/') bu[len] = '\0';
  
  c_fullbaseurl = fbu;
  c_baseurl     = bu;  
}

/************************************************************************/

static bool get_next(char *dest,const char **pp)
{
  const char *p;
  
  assert(dest != NULL);
  assert(pp   != NULL);
  assert(*pp  != NULL);
  
  p = *pp;
  
  if (*p == '\0') return false;
  while((*p != '\0') && (*p != '.'))
    *dest++ = *p++;
  
  assert((*p == '.') || (*p == '\0'));
  if (*p) p++;
  *dest = '\0';
  *pp = p;
  return true;
}

/************************************************************************/

static int get_field(
	lua_State  *const restrict L,
	const char *               name
)
{
  size_t len = strlen(name);
  char   field[len];
  
  lua_getglobal(L,"_G");
  while(get_next(field,&name))
  {
    lua_getfield(L,-1,field);
    lua_remove(L,-2);
    if (lua_isnil(L,-1)) break;
  }
  return lua_type(L,-1);
}

/***********************************************************************/

static const char *get_string(
	lua_State  *const restrict L,
	const char *const restrict name,
	const char *const restrict def
)
{
  char *val;
  
  assert(L    != NULL);
  assert(name != NULL);
  
  if (get_field(L,name) != LUA_TSTRING)
    val = (def != NULL) ? strdup(def) : NULL;
  else
    val = strdup(lua_tostring(L,-1));

  lua_pop(L,1);
  return val;
}

/*************************************************************************/

static int get_int(
	lua_State  *const restrict L,
	const char *const restrict name,
	const int                  def
)
{
  int val;
  
  assert(L    != NULL);
  assert(name != NULL);
  

  if (get_field(L,name) != LUA_TNUMBER)
    val = def;
  else
    val = lua_tointeger(L,-1);
    
  lua_pop(L,1);
  return val;
}

/*************************************************************************/

static bool get_bool(
	lua_State  *const restrict L,
	const char *const restrict name,
	const bool                 def
)
{
  bool val;
  
  assert(L    != NULL);
  assert(name != NULL);
  
  if (get_field(L,name) == LUA_TNIL)
    val = def;
  else
    val = lua_toboolean(L,-1);
  
  lua_pop(L,1);
  return val;
}

/**************************************************************************/
