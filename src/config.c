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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <syslog.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "config.h"
#include "conversion.h"

#if LUA_VERSION_NUM == 501
#  define lua_rawlen(L,idx)     lua_objlen((L),(idx))
#endif

/***************************************************************************/

static conversion__f confluaL_toconversion(lua_State *L,int idx,char const *def)
{
  assert(L   != NULL);
  assert(idx != 0);
  assert(def != NULL);
  
  char const *c = luaL_optstring(L,idx,def);
  if (strcmp(c,"text") == 0)
    return text_conversion;
  else if (strcmp(c,"mixed") == 0)
    return mixed_conversion;
  else if (strcmp(c,"html") == 0)
    return html_conversion;
  else if (strcmp(c,"none") == 0)
    return no_conversion;
  else
  {
    syslog(LOG_DEBUG,"conversion '%s' unsupported",c);
    return no_conversion;
  }
}

/***************************************************************************/

static void confluaL_tofields(lua_State *L,int idx,struct fields *fields)
{
  assert(L      != NULL);
  assert(idx    != 0);
  assert(fields != NULL);
  
  idx = lua_absindex(L,idx);
  if (lua_isnil(L,idx))
  {
    fields->uid  = 0;
    fields->name = 0;
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

static void confluaL_toauthor(lua_State *L,int idx,struct author *auth)
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
    confluaL_tofields(L,-1,&auth->fields);
    lua_pop(L,4);
  }
}

/***************************************************************************/

static void confluaL_toitems(lua_State *L,int idx,template__t *temp)
{
  assert(L    != NULL);
  assert(idx  != 0);
  assert(temp != NULL);
  
  if (lua_isnumber(L,idx))
  {
    temp->items = lua_tointeger(L,-1);
    temp->pagegen = pagegen_items;
  }
  else if (lua_isstring(L,idx))
  {
    char const *x = lua_tostring(L,-1);
    char       *p;
    
    temp->items = strtoul(x,&p,10);
    switch(*p)
    {
      case 'd': break;
      case 'w': temp->items *= 7; break;
      case 'm': temp->items *= 30; break;
      default:  break;
    }
    temp->pagegen = pagegen_days;
  }
  else if (lua_isnil(L,-1))
  {
    temp->items   = 15;
    temp->pagegen = pagegen_items;
  }
  else
  {
    syslog(LOG_ERR,"wrong type for items");
    temp->items   = 15;
    temp->pagegen = pagegen_items;
  }
}

/***************************************************************************/

static size_t confluaL_totemplates(lua_State *L,int idx,template__t **ptemps)
{
  size_t       max;
  template__t *temps;
  
  assert(L      != NULL);
  assert(idx    != 0);
  assert(ptemps != NULL);
  
  idx = lua_absindex(L,idx);
  if (lua_isnil(L,idx))
  {
    *ptemps = NULL;
    return 0;
  }
  
  max   = lua_rawlen(L,idx);
  temps = calloc(max,sizeof(template__t));
  if (temps == NULL)
  {
    syslog(LOG_ERR,"cannot alloate memory for templates");
    *ptemps = NULL;
    return 0;
  }
  
  for (size_t i = 0 ; i < max ; i++)
  {
    int tidx;
    
    lua_pushinteger(L,i + 1);
    lua_gettable(L,idx);
    tidx = lua_gettop(L);
    lua_getfield(L,tidx,"template");
    temps[i].template = luaL_checkstring(L,-1);
    lua_getfield(L,tidx,"output");
    temps[i].file = luaL_checkstring(L,-1);
    lua_getfield(L,tidx,"items");
    confluaL_toitems(L,-1,&temps[i]);
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

static void confluaL_toemail(lua_State *L,int idx,struct bemail *email)
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
    email->list = luaL_optstring(L,-1,NULL);
    lua_getfield(L,idx,"message");
    email->message = luaL_optstring(L,-1,NULL);
    lua_getfield(L,idx,"subject");
    email->subject = luaL_optstring(L,-1,NULL);
    lua_getfield(L,idx,"notify");
    if (lua_isnil(L,-1))
      email->notify = true;
    else
      email->notify = lua_toboolean(L,-1);
    lua_pop(L,4);
  }
}

/***************************************************************************/

static size_t confluaL_toaffiliates(lua_State *L,int idx,aflink__t **paffs)
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
  affs = calloc(max,sizeof(aflink__t));
  if (affs == NULL)
  {
    syslog(LOG_ERR,"cannot allocate memory for affiliates");
    *paffs = NULL;
    return 0;
  }
  
  for (size_t i = 0 ; i < max ; i++)
  {
    int tidx;
    
    lua_pushinteger(L,i + 1);
    lua_gettable(L,idx);
    tidx = lua_gettop(L);
    lua_getfield(L,tidx,"proto");
    affs[i].proto = luaL_checklstring(L,-1,&affs[i].psize);
    lua_getfield(L,tidx,"link");
    affs[i].format = luaL_checkstring(L,-1);
    lua_pop(L,3);
  }
  
  *paffs = affs;
  return max;
}

/***************************************************************************/

config__s *config_lua(char const *conf)
{
  config__s *config;
  lua_State *L;
  int        rc;
  
  config = malloc(sizeof(config__s));
  if (config == NULL)
  {
    syslog(LOG_ERR,"no memory to allocate config structure");
    free(config);
    return NULL;
  }
  
  if (conf == NULL)
  {
    conf = getenv("REDIRECT_BlOG_CONFIG");
    if (conf == NULL)
    {
      conf = getenv("BLOG_CONFIG");
      if (conf == NULL)
      {
        syslog(LOG_ERR,"env BLOG_CONFIG not defined");
        free(config);
        return NULL;
      }
    }
  }
  
  L = luaL_newstate();
  if (L == NULL)
  {
    syslog(LOG_ERR,"cannot create Lua state");
    free(config);
    return NULL;
  }

  lua_gc(L,LUA_GCSTOP,0);  
  luaL_openlibs(L);
  lua_gc(L,LUA_GCRESTART,0);
  
  rc = luaL_loadfile(L,conf);
  if (rc != LUA_OK)
  {
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,lua_tostring(L,-1));
    lua_close(L);
    free(config);
    return NULL;
  }
  
  rc = lua_pcall(L,0,LUA_MULTRET,0);
  if (rc != LUA_OK)
  {
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,lua_tostring(L,-1));
    lua_close(L);
    free(config);
    return NULL;
  }
  
  lua_getglobal(L,"name");
  config->name = luaL_optstring(L,-1,"A Blog Grows in Cyberspace");
  lua_getglobal(L,"description");
  config->description = luaL_optstring(L,-1,"A place where I talk about stuff in cyberspace");
  lua_getglobal(L,"class");
  config->class = luaL_optstring(L,-1,"blog, rants, random stuff, programming");
  lua_getglobal(L,"basedir");
  config->basedir = luaL_optstring(L,-1,"/path/to/basedir");
  lua_getglobal(L,"lockfile");
  config->lockfile = luaL_optstring(L,-1,"/tmp/.mod_blog.lock");
  lua_getglobal(L,"webdir");
  config->webdir = luaL_optstring(L,-1,"/path/to/webdir");
  lua_getglobal(L,"url");
  config->url = luaL_optstring(L,-1,"http://www.example.com/blog/");
  lua_getglobal(L,"prehook");
  config->prehook = luaL_optstring(L,-1,"/path/to/prehook");
  lua_getglobal(L,"posthook");
  config->posthook = luaL_optstring(L,-1,"/path/to/posthook");
  lua_getglobal(L,"adtag");
  config->adtag = luaL_optstring(L,-1,"programming");
  lua_getglobal(L,"conversion");
  config->conversion = confluaL_toconversion(L,-1,"html");
  lua_getglobal(L,"author");
  confluaL_toauthor(L,-1,&config->author);
  lua_getglobal(L,"templates");
  config->templatenum = confluaL_totemplates(L,-1,&config->templates);
  lua_getglobal(L,"email");
  confluaL_toemail(L,-1,&config->email);
  lua_getglobal(L,"affiliate");
  config->affiliatenum = confluaL_toaffiliates(L,-1,&config->affiliates);
  lua_pop(L,15);
  
  config->user = L;
  return config;
}

/***************************************************************************/

void config_free(config__s *config)
{
  assert(config != NULL);
  
  free(config->affiliates);
  free(config->templates);
  lua_close(config->user);  
  free(config);
}

/***************************************************************************/
