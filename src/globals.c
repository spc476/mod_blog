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
#include <errno.h>
#include <assert.h>

#include <syslog.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <cgilib6/url.h>
#include <cgilib6/util.h>

#include "conf.h"
#include "conversion.h"
#include "frontend.h"
#include "backend.h"
#include "fix.h"
#include "timeutil.h"
#include "blog.h"

/***********************************************************/

void	set_c_updatetype	(const char *const);
void	set_gf_emailupdate	(const char *const);
void	set_c_conversion	(const char *const);
void	set_c_url		(const char *const);

static bool	   get_next	(char *,const char **);
static int	   get_field	(lua_State *const,const char *);
static const char *get_string	(lua_State *const,const char *const restrict,const char *const restrict);
static int         get_int	(lua_State *const,const char *const,const int);
static bool	   get_bool	(lua_State *const,const char *const,const bool);
static void	   globals_free	(void);

/************************************************************/

const char   *c_name;				/* no free */
const char   *c_basedir;			/* no free */
const char   *c_webdir;				/* no free */
char         *c_baseurl;
char         *c_fullbaseurl;
char         *c_htmltemplates;
int           c_days;
const char   *c_author;				/* no free */
const char   *c_email;				/* no free */
const char   *c_authorfile;			/* no free */
size_t        c_af_uid;
size_t        c_af_name;
const char   *c_updatetype   = "NewEntry";	/* no free */
const char   *c_lockfile;			/* no free */
const char   *c_emaildb;			/* no free */
const char   *c_emailsubject;			/* no free */
const char   *c_emailmsg;			/* no free */
int           c_tzhour;
int           c_tzmin;
const char  *c_overview;			/* no free */
void	    (*c_conversion)(FILE *,FILE *) =  html_conversion;
bool          cf_emailupdate = true;
template__t  *c_templates;
size_t        c_numtemplates;
aflink__t    *c_aflinks;
size_t        c_numaflinks;
const char   *c_adtag;				/* no free */

lua_State     *g_L;
const char    *g_templates;
volatile bool  gf_debug       = false;
Blog           g_blog;
struct display gd =
{
  .navunit = UNIT_INDEX,
  .f       = 
  { 
    .fullurl    = false ,
    .reverse    = false ,
    .navigation = false ,
    .navprev    = true  ,
    .navnext    = true  ,
    .edit       = false ,
    .overview   = false ,
  } ,
};

/****************************************************/

#if LUA_VERSION_NUM > 501
static size_t lua_objlen(lua_State *L,int index)
{
  size_t len;
  
  lua_len(L,index);
  len = lua_tointeger(L,-1);
  lua_pop(L,1);
  return len;
}
#endif

/****************************************************/

int GlobalsInit(const char *conf)
{
  int rc;
  
  atexit(globals_free);
  
  if (conf == NULL)
  {
    conf = getenv("REDIRECT_BLOG_CONFIG");
    if (conf == NULL)
    {
      conf = getenv("BLOG_CONFIG");
      if (conf == NULL)
      {
        syslog(LOG_ERR,"env BLOG_CONFIG not defined");
        return ENOMSG;
      }
    }
  }
  
  g_L = luaL_newstate();
  if (g_L == NULL)
  {
    syslog(LOG_ERR,"cannot create Lua state");
    return ENOMEM;
  }
  
  lua_gc(g_L,LUA_GCSTOP,0);
  luaL_openlibs(g_L);
  lua_gc(g_L,LUA_GCRESTART,0);
  rc = luaL_loadfile(g_L,conf);
  
  if (rc != 0)
  {
    const char *err = lua_tostring(g_L,-1);
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,err);
    return -1;
  }
  
  rc = lua_pcall(g_L,0,LUA_MULTRET,0);
  if (rc != 0)
  {
    const char *err = lua_tostring(g_L,-1);
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,err);
    return -1;
  }
  
  c_name         = get_string(g_L,"name",NULL);
  c_basedir      = get_string(g_L,"basedir",NULL);
  c_webdir       = get_string(g_L,"webdir",NULL);
  c_lockfile     = get_string(g_L,"lockfile","/tmp/.mod_blog.lock");
  c_overview     = get_string(g_L,"overview",NULL);
  c_author       = get_string(g_L,"author.name",NULL);
  c_email        = get_string(g_L,"author.email",NULL);
  c_authorfile   = get_string(g_L,"author.file",NULL);
  c_af_uid       = get_int   (g_L,"author.fields.uid",1)  - 1;
  c_af_name      = get_int   (g_L,"author.fields.name",3) - 1;
  c_emaildb      = get_string(g_L,"email.list",NULL);
  c_emailsubject = get_string(g_L,"email.subject",NULL);
  c_emailmsg     = get_string(g_L,"email.message",NULL);
  c_adtag        = get_string(g_L,"adtag","programming");  
  cf_emailupdate = get_bool  (g_L,"email.notify",true);    
  
  gf_debug       = get_bool  (g_L,"debug",false);  
  gd.f.overview  = (c_overview != NULL);  
  
  if (c_emaildb == NULL)
    cf_emailupdate = false;
  
  {
    const char *timezone = get_string(g_L,"timezone","-5:00");
    char       *p;
  
    c_tzhour = strtol(timezone,&p,10);
    p++;
    c_tzmin  = strtoul(p,NULL,10);
  }
  
  set_c_conversion(get_string(g_L,"conversion","html"));
  set_c_url       (get_string(g_L,"url",NULL));
  
  /*------------------------------------------------
  ; process the templates array
  ;-------------------------------------------------*/
  
  lua_getglobal(g_L,"templates");
  if (lua_isnil(g_L,-1))
  {
    syslog(LOG_ERR,"missing templates section");
    return ENOENT;
  }
  
  c_numtemplates = lua_objlen(g_L,-1);
  
  if (c_numtemplates == 0)
  {
    syslog(LOG_ERR,"no templates");
    return ENOENT;
  }
  
  c_templates   = malloc(sizeof(template__t) * c_numtemplates);
  if (c_templates == NULL)
  {
    syslog(LOG_ERR,"%s",strerror(ENOMEM));
    return ENOMEM;
  }
  
  assert(c_numtemplates >= 1);
  
  for (size_t i = 0 ; i < c_numtemplates; i++)
  {
    lua_pushinteger(g_L,i + 1);
    lua_gettable(g_L,-2);
    
    lua_getfield(g_L,-1,"template");
    lua_getfield(g_L,-2,"reverse");
    lua_getfield(g_L,-3,"fullurl");
    lua_getfield(g_L,-4,"output");
    lua_getfield(g_L,-5,"items");
    
    c_templates[i].template = lua_tostring(g_L,-5);
    c_templates[i].reverse  = lua_toboolean(g_L,-4);
    c_templates[i].fullurl  = lua_toboolean(g_L,-3);
    c_templates[i].file     = lua_tostring(g_L,-2);
    
    if (lua_isnumber(g_L,-1))
    {
      c_templates[i].items = lua_tointeger(g_L,-1);
      c_templates[i].pagegen = pagegen_items;
    }
    else if (lua_isstring(g_L,-1))
    {
      const char *x;
      char       *p;
      
      x = lua_tostring(g_L,-1);
      c_templates[i].items = strtoul(x,&p,10);
      switch(*p)
      {
        case 'd': break;
        case 'w': c_templates[i].items *=  7; break;
        case 'm': c_templates[i].items *= 30; break;
        default:  break;
      }
      c_templates[i].pagegen = pagegen_days;
    }
    else if (lua_isnil(g_L,-1))
    {
      c_templates[i].items   = 15;
      c_templates[i].pagegen = pagegen_items;
    }
    else
    {
      syslog(LOG_ERR,"wrong type for items");
      return EINVAL;
    }
    
    lua_pop(g_L,6);
  }  
  lua_pop(g_L,1);
  
  c_htmltemplates = strdup(c_templates[0].template);
  g_templates     = c_htmltemplates;
  c_days          = c_templates[0].items;
  
  /*----------------------------------------------------
  ; process affiliate links
  ;----------------------------------------------------*/
  
  lua_getglobal(g_L,"affiliate");
  if (!lua_isnil(g_L,-1))
  {
    c_numaflinks = lua_objlen(g_L,-1);
    c_aflinks    = malloc(sizeof(aflink__t) * c_numaflinks);
    
    if (c_aflinks == NULL)
    {
      syslog(LOG_ERR,"%s",strerror(ENOMEM));
      return ENOMEM;
    }
    
    for (size_t i = 0 ; i < c_numaflinks ; i++)
    {
      lua_pushinteger(g_L,i + 1);
      lua_gettable(g_L,-2);
      
      lua_getfield(g_L,-1,"proto");
      lua_getfield(g_L,-2,"link");
      
      c_aflinks[i].proto  = lua_tolstring(g_L,-2,&c_aflinks[i].psize);
      c_aflinks[i].format = lua_tostring(g_L,-1);
      
      lua_pop(g_L,3);
    }
  }
  lua_pop(g_L,1);
  
  g_blog = BlogNew(c_basedir,c_lockfile);
  if (g_blog == NULL)
    return ENOMEM;
  
  /*-------------------------------------------------------
  ; for most sorting routines, I just assume C sorting
  ; conventions---this makes sure I have those for sorting
  ; and searching only.
  ;--------------------------------------------------------*/
  
  setlocale(LC_COLLATE,"C");  
  return 0;
}

/********************************************************************/

void set_c_updatetype(const char *const value)
{
  if (value == NULL) return;
  if (empty_string(value)) return;
  
  if (strcmp(value,"new") == 0)
    c_updatetype = "NewEntry";
  else if (strcmp(value,"modify") == 0)
    c_updatetype = "ModifiedEntry";
  else if (strcmp(value,"exit") == 0)
    c_updatetype = "ModifiedEntry";
  else if (strcmp(value,"template") == 0)
    c_updatetype = "TemplateChange";
  else 
    c_updatetype = "Other";
}

/************************************************************************/

void set_cf_emailupdate(const char *const value)
{
  if (value && !empty_string(value))
  {
    if (strcmp(value,"no") == 0)
      cf_emailupdate = false;
    else if (strcmp(value,"yes") == 0)
      cf_emailupdate = true;
  }
}

/***************************************************************************/

void set_c_conversion(const char *const value)
{
  if (value == NULL) return;
  if (empty_string(value)) return;
  
  if (strcmp(value,"text") == 0)
    c_conversion = text_conversion;
  else if (strcmp(value,"mixed") == 0)
    c_conversion = mixed_conversion;
  else if (strcmp(value,"html") == 0)
    c_conversion = html_conversion;
  else if (strcmp(value,"none") == 0)
    c_conversion = no_conversion;
}

/**************************************************************************/

void set_c_url(const char *const turl)
{
  url__t *url;
  size_t  len;
  char   *fbu;
  char   *bu;
  
  assert(turl != NULL);
  
  fbu = strdup(turl);
  url = UrlNew(turl);
  
  if (url == NULL)
  {
    syslog(LOG_ERR,"unparsable URL");
    return;
  }
  
  if (url->scheme == URL_HTTP)
    bu = strdup(url->http.path);
  else if (url->scheme == URL_GOPHER)
    bu = strdup(url->gopher.selector);
  else
  {
    syslog(LOG_WARNING,"unsupported URL type");
    return;
  }
  
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
  UrlFree(url);
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
	lua_State  *const L,
	const char *      name
)
{
  size_t len = strlen(name);
  char   field[len];
  
  assert(L    != NULL);
  assert(name != NULL);
  
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
	lua_State  *const          L,
	const char *const restrict name,
	const char *const restrict def
)
{
  const char *val;
  
  assert(L    != NULL);
  assert(name != NULL);
  
  if (get_field(L,name) != LUA_TSTRING)
    val = def;
  else
    val = lua_tostring(L,-1);
  
  lua_pop(L,1);
  return val;
}

/*************************************************************************/

static int get_int(
        lua_State  *const L,
        const char *const name,
        const int         def
)
{
  int val;
  
  if (get_field(L,name) != LUA_TNUMBER)
    val = def;
  else
    val = lua_tointeger(L,-1);
  
  lua_pop(L,1);
  return val;
}

/*************************************************************************/

static bool get_bool(
	lua_State  *const L,
	const char *const name,
	const bool        def
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

static void globals_free(void)
{
  if (gd.cgi != NULL)
  {
    CgiFree(gd.cgi);
    gd.cgi = NULL;
  }
  
  if (gd.req.in  != stdin)  fclose(gd.req.in);
  if (gd.req.out != stdout) fclose(gd.req.out);

  free(gd.req.update);
  free(gd.req.origauthor);
  free(gd.req.author);
  free(gd.req.title);
  free(gd.req.class);
  free(gd.req.status);
  free(gd.req.date);
  free(gd.req.origbody);
  free(gd.req.body);
  
  if (g_blog != NULL) BlogFree(g_blog);
  if (g_L    != NULL) lua_close(g_L);
  
  free(c_aflinks);  
  free(c_templates);    
  free(c_baseurl);
  free(c_fullbaseurl);
  free(c_htmltemplates);
}

/***********************************************************************/
