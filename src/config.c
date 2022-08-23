/**************************************************************
*
* Copyright 2022 by Sean Conner.  All Rights Reserved.
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
****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <syslog.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "config.h"

#if LUA_VERSION_NUM == 501
#  define LUA_OK                0
#  define lua_rawlen(L,idx)     lua_objlen((L),(idx))

   int lua_absindex(lua_State *L,int idx)
   {
     return (idx > 0) || (idx <= LUA_REGISTRYINDEX)
            ? idx
            : lua_gettop(L) + idx + 1
            ;
   }
#endif

/***************************************************************************/

static char const *confL_checklstring(lua_State *L,int idx,size_t *ps,char const *table,char const *field)
{
  assert(L     != NULL);
  assert(idx   != 0);
  assert(table != NULL);
  assert(field != NULL);
  
  char const *s = lua_tolstring(L,idx,ps);
  if (s == NULL)
    luaL_error(L,"table '%s' missing mandatory field '%s'",table,field);
  return s;
}

/***************************************************************************/

static char const *confL_checkstring(lua_State *L,int idx,char const *table,char const *field)
{
  assert(L     != NULL);
  assert(idx   != 0);
  assert(table != NULL);
  assert(field != NULL);
  
  return confL_checklstring(L,idx,NULL,table,field);
}

/***************************************************************************/

static void confL_tofields(lua_State *L,int idx,struct fields *fields)
{
  assert(L      != NULL);
  assert(idx    != 0);
  assert(fields != NULL);
  
  idx = lua_absindex(L,idx);
  if (lua_isnil(L,idx))
  {
    fields->uid   = 0;
    fields->name  = 0;
    fields->email = 0;
  }
  else
  {
    lua_getfield(L,idx,"uid");
    fields->uid = luaL_optinteger(L,-1,1) - 1;
    lua_getfield(L,idx,"name");
    fields->name = luaL_optinteger(L,-1,3) - 1;
    lua_getfield(L,idx,"email");
    fields->email = luaL_optinteger(L,-1,4) - 1;
    lua_pop(L,3);
  }
}

/***************************************************************************/

static void confL_toauthor(lua_State *L,int idx,struct author *auth)
{
  assert(L    != NULL);
  assert(idx  != 0);
  assert(auth != NULL);
  
  idx = lua_absindex(L,idx);
  if (lua_isnil(L,idx))
  {
    auth->name        = "Joe Blog";
    auth->email       = "joe@example.com";
    auth->file        = NULL;
    auth->fields.uid  = 0;
    auth->fields.name = 0;
  }
  else
  {
    lua_getfield(L,idx,"name");
    auth->name = luaL_optstring(L,-1,"Joe Blog");
    lua_getfield(L,idx,"email");
    auth->email = luaL_optstring(L,-1,"joe@example.com");
    lua_getfield(L,idx,"file");
    auth->file = luaL_optstring(L,-1,NULL);
    lua_getfield(L,idx,"fields");
    confL_tofields(L,-1,&auth->fields);
    lua_pop(L,4);
  }
}

/***************************************************************************/

static void confL_toemail(lua_State *L,int idx,struct bemail *email)
{
  assert(L     != NULL);
  assert(idx   != 0);
  assert(email != NULL);
  
  idx = lua_absindex(L,idx);
  if (lua_isnil(L,idx))
  {
    email->list    = NULL;
    email->message = NULL;
    email->subject = NULL;
    email->notify  = false;
  }
  else
  {
    lua_getfield(L,idx,"list");
    email->list = confL_checkstring(L,-1,"email","list");
    lua_getfield(L,idx,"message");
    email->message = confL_checkstring(L,-1,"email","message");
    lua_getfield(L,idx,"subject");
    email->subject = confL_checkstring(L,-1,"email","subject");
    email->notify  = true;
    lua_pop(L,3);
  }
}

/***************************************************************************/

static void confL_toitems(lua_State *L,int idx,template__t *temp)
{
  assert(L    != NULL);
  assert(idx  != 0);
  assert(temp != NULL);
  
  if (lua_isnumber(L,idx))
  {
    temp->items = lua_tointeger(L,-1);
    temp->pagegen = "items";
  }
  else if (lua_isstring(L,idx))
  {
    char const *x = lua_tostring(L,-1);
    char       *p;
    
    temp->items = strtoul(x,&p,10);
    switch(*p)
    {
      case 'd': break;
      case 'w': temp->items *=  7; break;
      case 'm': temp->items *= 30; break;
      default:  break;
    }
    
    temp->pagegen = "days";
  }
  else if (lua_isnil(L,-1))
  {
    temp->items   = 15;
    temp->pagegen = "items";
  }
  else
    luaL_error(L,"wrong type for items");
}

/***************************************************************************/

static size_t confL_totemplates(lua_State *L,int idx,template__t **ptemps)
{
  size_t       max;
  template__t *temps;
  
  assert(L      != NULL);
  assert(idx    != 0);
  assert(ptemps != NULL);
  
  idx = lua_absindex(L,idx);
  if (lua_isnil(L,idx))
    return luaL_error(L,"missing template secton");
    
  max = lua_rawlen(L,idx);
  
  if (max == 0)
    return luaL_error(L,"at least one template file is required");
    
  temps = lua_newuserdata(L,max * sizeof(template__t));
  lua_pushvalue(L,-1);
  luaL_ref(L,LUA_REGISTRYINDEX); /* prevent GC reclaiming this memory  */
  
  for (size_t i = 0 ; i < max ; i++)
  {
    int tidx;
    
    lua_pushinteger(L,i + 1);
    lua_gettable(L,idx);
    tidx = lua_gettop(L);
    lua_getfield(L,tidx,"template");
    temps[i].template = confL_checkstring(L,-1,"template","template");
    lua_getfield(L,tidx,"output");
    temps[i].file = confL_checkstring(L,-1,"template","output");
    lua_getfield(L,tidx,"items");
    confL_toitems(L,-1,&temps[i]);
    lua_getfield(L,tidx,"reverse");
    temps[i].reverse = lua_toboolean(L,-1);
    lua_getfield(L,tidx,"fullurl");
    temps[i].fullurl = lua_toboolean(L,-1);
    lua_getfield(L,tidx,"posthook");
    temps[i].posthook = luaL_optstring(L,-1,NULL);
    lua_pop(L,7);
  }
  
  *ptemps = temps;
  return max;
}

/***************************************************************************/

static size_t confL_toaffiliates(lua_State *L,int idx,aflink__t **paffs)
{
  size_t     max;
  aflink__t *affs;
  
  assert(L     != NULL);
  assert(idx   != 0);
  assert(paffs != NULL);
  
  idx = lua_absindex(L,idx);
  if (lua_isnil(L,idx))
  {
    *paffs = NULL;
    return 0;
  }
  
  max = lua_rawlen(L,idx);
  if (max == 0)
  {
    *paffs = NULL;
    return 0;
  }
  
  affs = lua_newuserdata(L,max * sizeof(aflink__t));
  lua_pushvalue(L,-1);
  luaL_ref(L,LUA_REGISTRYINDEX); /* prevent GC reclaiming this memory */
  
  for (size_t i = 0 ; i < max ; i++)
  {
    int tidx;
    
    lua_pushinteger(L,i + 1);
    lua_gettable(L,idx);
    tidx = lua_gettop(L);
    lua_getfield(L,tidx,"proto");
    affs[i].proto = confL_checklstring(L,-1,&affs[i].psize,"affiliate","proto");
    lua_getfield(L,tidx,"link");
    affs[i].format = confL_checkstring(L,-1,"affiliate","link");
    lua_pop(L,3);
  }
  
  *paffs = affs;
  return max;
}

/***************************************************************************/

static int confL_config(lua_State *L)
{
  size_t urllen;
  
  assert(L != NULL);
  
  config__s *config = lua_touserdata(L,1);
  lua_getglobal(L,"name");
  config->name = luaL_optstring(L,-1,"A Blog Grows in Cyberspace");
  lua_getglobal(L,"description");
  config->description = luaL_optstring(L,-1,"A place where I talk about stuff in cyberspace");
  lua_getglobal(L,"class");
  config->class = luaL_optstring(L,-1,"blog, rants, random stuff, programming");
  lua_getglobal(L,"basedir");
  config->basedir = luaL_optstring(L,-1,".");
  lua_getglobal(L,"webdir");
  config->webdir = luaL_optstring(L,-1,"htdocs");
  lua_getglobal(L,"lockfile");
  config->lockfile = luaL_optstring(L,-1,".mod_blog.lock");
  lua_getglobal(L,"url");
  config->url = luaL_optlstring(L,-1,"http://www.example.com/blog/",&urllen);
  lua_getglobal(L,"adtag");
  config->adtag = luaL_optstring(L,-1,"programming");
  lua_getglobal(L,"conversion");
  config->conversion = luaL_optstring(L,-1,"html");
  lua_getglobal(L,"prehook");
  config->prehook = luaL_optstring(L,-1,NULL);
  lua_getglobal(L,"posthook");
  config->posthook = luaL_optstring(L,-1,NULL);
  lua_getglobal(L,"author");
  confL_toauthor(L,-1,&config->author);
  lua_getglobal(L,"email");
  confL_toemail(L,-1,&config->email);
  lua_getglobal(L,"templates");
  config->templatenum = confL_totemplates(L,-1,&config->templates);
  lua_getglobal(L,"affiliate");
  config->affiliatenum = confL_toaffiliates(L,-1,&config->affiliates);
  
  /*--------------------------------------
  ; ensure that the URL ends with a '/'.
  ;---------------------------------------*/
  
  if (config->url[urllen-1] != '/')
  {
    lua_pushlstring(L,config->url,urllen);
    lua_pushliteral(L,"/");
    lua_concat(L,2);
    config->url = lua_tostring(L,-1);
  }
  
  /*---------------------------------------------------
  ; XXX - need to think about a messed up URL here ...
  ;----------------------------------------------------*/
  
  char const *s = strchr(config->url,'/'); /* first slash in authority section  */
  s = strchr(++s,'/'); /* second slash in authority section */
  s = strchr(++s,'/'); /* start of path component */
  config->baseurl = s;
  return 0;
}

/***************************************************************************/

config__s *config_lua(char const *conf)
{
  config__s *config;
  lua_State *L;
  int        rc;
  
  if (conf == NULL)
  {
    conf = getenv("REDIRECT_BlOG_CONFIG");
    if (conf == NULL)
    {
      conf = getenv("BLOG_CONFIG");
      if (conf == NULL)
      {
        syslog(LOG_ERR,"env BLOG_CONFIG not defined");
        return NULL;
      }
    }
  }
  
  config = malloc(sizeof(config__s));
  if (config == NULL)
  {
    syslog(LOG_ERR,"no memory to allocate config structure");
    return NULL;
  }
  
  L = luaL_newstate();
  if (L == NULL)
  {
    syslog(LOG_ERR,"cannot create Lua state");
    free(config);
    return NULL;
  }
  
  luaL_openlibs(L);
  
  rc = luaL_loadfile(L,conf);
  if (rc != LUA_OK)
  {
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,lua_tostring(L,-1));
    lua_close(L);
    free(config);
    return NULL;
  }
  
  rc = lua_pcall(L,0,0,0);
  if (rc != LUA_OK)
  {
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,lua_tostring(L,-1));
    lua_close(L);
    free(config);
    return NULL;
  }
  
  /*-----------------------------------------------------------------------
  ; Some items aren't optional, so let Lua do the error checking for us and
  ; return an error, which is caught here; otherwise, things might blow up.
  ; Besides, catching common configuration errors is always a good thing.
  ;------------------------------------------------------------------------*/
  
  lua_pushcfunction(L,confL_config);
  lua_pushlightuserdata(L,config);

  rc = lua_pcall(L,1,0,0);
  if (rc != LUA_OK)
  {
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,lua_tostring(L,-1));
    lua_close(L);
    free(config);
    return NULL;
  }
  
  config->user = L;
  return config;
}

/***************************************************************************/

void config_free(config__s *config)
{
  assert(config       != NULL);
  assert(config->user != NULL);
  
  lua_close(config->user);
  free(config);
}

/***************************************************************************/
