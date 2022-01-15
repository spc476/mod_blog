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

#include <stdint.h>
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
#include "timeutil.h"
#include "blog.h"

#define MAX_ITEMS(type) (((SIZE_MAX - sizeof(type)) + 1) / sizeof(type))

/***********************************************************/

extern void set_c_updatetype   (char const *);
extern void set_gf_emailupdate (char const *);
extern void set_c_conversion   (char const *);
extern void set_c_url          (char const *);

static bool        get_next     (char const **,size_t *);
static int         get_field    (lua_State *,char const *);
static char const *get_string   (lua_State *,char const *restrict,char const *restrict);
static int         get_int      (lua_State *,char const *,int);
static bool        get_bool     (lua_State *,char const *,bool);
static void	   seed_rng	(void);
static void        globals_free (void);

/************************************************************/

char const   *c_name;                    /* no free */
char const   *c_description;             /* no free */
char const   *c_class;                   /* no free */
char const   *c_basedir;                 /* no free */
char const   *c_webdir;                  /* no free */
char const   *c_author;                  /* no free */
char const   *c_email;                   /* no free */
char const   *c_authorfile;              /* no free */
char const   *c_updatetype = "NewEntry"; /* no free */
char const   *c_lockfile;                /* no free */
char const   *c_emaildb;                 /* no free */
char const   *c_emailsubject;            /* no free */
char const   *c_emailmsg;                /* no free */
char const   *c_overview;                /* no free */
char const   *c_adtag;                   /* no free */
char const   *c_prehook;                 /* no free */
char const   *c_posthook;                /* no free */
char         *c_baseurl;
char         *c_fullbaseurl;
char         *c_htmltemplates;
template__t  *c_templates;
aflink__t    *c_aflinks;
void        (*c_conversion)(FILE *restrict,FILE *restrict) =  html_conversion;
int           c_days;
size_t        c_af_uid;
size_t        c_af_name;
bool          cf_emailupdate = true;
size_t        c_numtemplates;
size_t        c_numaflinks;

lua_State      *g_L;
char const     *g_templates;
bool volatile   gf_debug = false;
Blog           *g_blog;
struct display  gd =
{
  .navunit = UNIT_PART,
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

#if LUA_VERSION_NUM == 501
#  define lua_rawlen(L,idx)	lua_objlen((L),(idx))
#endif

/****************************************************/

int GlobalsInit(char const *conf)
{
  int rc;
  
  atexit(globals_free);
  seed_rng();
  
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
    char const *err = lua_tostring(g_L,-1);
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,err);
    return -1;
  }
  
  rc = lua_pcall(g_L,0,LUA_MULTRET,0);
  if (rc != 0)
  {
    char const *err = lua_tostring(g_L,-1);
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,err);
    return -1;
  }
  
  c_name         = get_string(g_L,"name",NULL);
  c_description  = get_string(g_L,"description",NULL);
  c_class        = get_string(g_L,"class",NULL);
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
  c_prehook      = get_string(g_L,"prehook",NULL);
  c_posthook     = get_string(g_L,"posthook",NULL);
  cf_emailupdate = get_bool  (g_L,"email.notify",true);
  
  gf_debug       = get_bool  (g_L,"debug",false);
  gd.f.overview  = (c_overview != NULL);
  
  if (c_emaildb == NULL)
    cf_emailupdate = false;
    
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
  
  c_numtemplates = lua_rawlen(g_L,-1);
  
  if (c_numtemplates == 0)
  {
    syslog(LOG_ERR,"no templates");
    return ENOENT;
  }
  
  if (c_numtemplates > MAX_ITEMS(template__t))
  {
    syslog(LOG_ERR,"too man templates");
    return ENOMEM;
  }
  
  c_templates = calloc(c_numtemplates,sizeof(template__t));
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
    
    lua_getfield(g_L,-1,"posthook");
    lua_getfield(g_L,-2,"template");
    lua_getfield(g_L,-3,"reverse");
    lua_getfield(g_L,-4,"fullurl");
    lua_getfield(g_L,-5,"output");
    lua_getfield(g_L,-6,"items");
    
    c_templates[i].posthook = lua_tostring(g_L,-6);
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
      char const *x;
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
    
    lua_pop(g_L,7);
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
    c_numaflinks = lua_rawlen(g_L,-1);
    
    if (c_numaflinks > 0)
    {
      if (c_numaflinks > MAX_ITEMS(aflink__t))
      {
        syslog(LOG_ERR,"too many affiliate links");
        return ENOMEM;
      }
      
      c_aflinks = calloc(c_numaflinks,sizeof(aflink__t));
      
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

void set_c_updatetype(char const *value)
{
  if (value == NULL) return;
  if (empty_string(value)) return;
  
  if (strcmp(value,"new") == 0)
    c_updatetype = "NewEntry";
  else if (strcmp(value,"modify") == 0)
    c_updatetype = "ModifiedEntry";
  else if (strcmp(value,"edit") == 0)
    c_updatetype = "ModifiedEntry";
  else if (strcmp(value,"template") == 0)
    c_updatetype = "TemplateChange";
  else
    c_updatetype = "Other";
}

/************************************************************************/

void set_cf_emailupdate(char const *value)
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

void set_c_conversion(char const *value)
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

void set_c_url(char const *turl)
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
    free(fbu);
    syslog(LOG_ERR,"unparsable URL");
    return;
  }
  
  if (url->scheme == URL_HTTP)
    bu = strdup(url->http.path);
  else if (url->scheme == URL_GOPHER)
    bu = strdup(url->gopher.selector);
  else
  {
    free(fbu);
    UrlFree(url);
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

static bool get_next(char const **pp,size_t *plen)
{
  assert(pp   != NULL);
  assert(*pp  != NULL);
  assert(plen != NULL);
  
  if (**pp == '\0') return false;
  
  const char *p   = *pp;
  size_t      len = 0;
  
  while((*p != '\0') && (*p != '.'))
  {
    p++;
    len++;
  }
  
  assert((*p == '.') || (*p == '\0'));
  if (*p) p++;
  *pp   = p;
  *plen = len;
  return true;
}

/************************************************************************/

static int get_field(lua_State  *L,char const *name)
{
  const char *field;
  size_t      len;
  
  assert(L    != NULL);
  assert(name != NULL);
  
  field = name;
  lua_getglobal(L,"_G");
  
  while(get_next(&name,&len))
  {
    lua_pushlstring(L,field,len);
    lua_gettable(L,-2);
    lua_remove(L,-2);
    if (lua_isnil(L,-1)) break;
    field = name;
  }
  return lua_type(L,-1);
}

/***********************************************************************/

static char const *get_string(lua_State  *L,char const *restrict name,char const *restrict def)
{
  char const *val;
  
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

static int get_int(lua_State  *L,char const *name,int  const  def)
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

static bool get_bool(lua_State  *L,char const *name,bool const  def)
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

static void seed_rng(void)
{
  FILE         *fp;
  unsigned int  seed;
  
  fp = fopen("/dev/urandom","rb");
  if (fp)
  {
    fread(&seed,sizeof(seed),1,fp);
    fclose(fp);
  }
  
  srand(seed);
}

/**************************************************************************/

static void globals_free(void)
{
  if (gd.cgi != NULL)
  {
    CgiFree(gd.cgi);
    gd.cgi = NULL;
  }
  
  fclose(gd.req.in);
  fclose(gd.req.out);
  free(gd.req.update);
  free(gd.req.origauthor);
  free(gd.req.author);
  free(gd.req.title);
  free(gd.req.class);
  free(gd.req.status);
  free(gd.req.date);
  free(gd.req.adtag);
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
