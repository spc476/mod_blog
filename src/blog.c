/***************************************************
*
* Copyright 2007 by Sean Conner.  All Rights Reserved.
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
****************************************************/

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>

#include <lualib.h>
#include <lauxlib.h>

#include "blog.h"
#include "wbtum.h"

/***********************************************************************/

static inline size_t max(size_t a,size_t b)
{
  if (a > b)
    return a;
  else
    return b;
}

/***********************************************************************/

static bool set_date(char const *file,struct btm *when,struct btm *now)
{
  assert(file != NULL);
  assert(when != NULL);
  assert(now  != NULL);
  
  FILE *fp = fopen(file,"r");
  if (fp == NULL)
  {
    fp = fopen(file,"w");
    if (fp != NULL)
    {
      *when = *now;
      fprintf(
        fp,
        "%4d/%02d/%02d.%d\n",
        now->year,
        now->month,
        now->day,
        now->part
      );
      fclose(fp);
    }
    else
    {
      syslog(LOG_ERR,"%s: %s",file,strerror(errno));
      return false;
    }
  }
  else
  {
    char buffer[128];
    char *p;
    
    fgets(buffer,sizeof(buffer),fp);
    when->year  = strtoul(buffer,&p,10); p++;
    when->month = strtoul(p,&p,10); p++;
    when->day   = strtoul(p,&p,10); p++;
    when->part  = strtoul(p,&p,10);
    fclose(fp);
  }
  
  return true;
}

/***********************************************************************/

static int blog_lock(char const *lockfile)
{
  if (lockfile)
  {
    int lock = open(lockfile,O_CREAT | O_RDWR,0x666);
    if (lock == -1)
    {
      syslog(LOG_ERR,"%s %s",lockfile,strerror(errno));
      return lock;
    }
    
    int rc = fcntl(
                    lock,
                    F_SETLKW,
                    &(struct flock){
                      .l_type   = F_WRLCK,
                      .l_start  = 0,
                      .l_whence = SEEK_SET,
                      .l_len    = 0,
                    }
                  );
    if (rc < 0)
    {
      syslog(LOG_ERR,"fcntl('%s',F_SETLKW) = %s",lockfile,strerror(errno));
      close(lock);
      return -1;
    }
    
    return lock;
  }
  
  return -1;
}

/***********************************************************************/

static void blog_unlock(char const *lockfile,int lock)
{
  assert(lockfile != NULL);
  
  if (lock >= 0)
  {
    int rc = fcntl(
               lock,
               F_SETLK,
               &(struct flock){
                 .l_type   = F_UNLCK,
                 .l_start  = 0,
                 .l_whence = SEEK_SET,
                 .l_len    = 0,
              }
            );
    if (rc == 0)
    {
      close(lock);
      remove(lockfile);
    }
    else
      syslog(LOG_ERR,"fcntl('%s',F_UNLCK) = %s",lockfile,strerror(errno));
  }
}

/***********************************************************************/

static void date_to_dir(char *tname,struct btm const *date)
{
  assert(tname != NULL);
  assert(date  != NULL);
  
  snprintf(tname,FILENAME_MAX,"%04d/%02d/%02d",date->year,date->month,date->day);
}

/***********************************************************************/

static void date_to_filename(char *tname,struct btm const *date,char const *file)
{
  assert(tname != NULL);
  assert(date  != NULL);
  assert(file  != NULL);
  
  snprintf(tname,FILENAME_MAX,"%04d/%02d/%02d/%s",date->year,date->month,date->day,file);
}

/**********************************************************************/

static void date_to_part(char *tname,struct btm const *date,int p)
{
  assert(tname != NULL);
  assert(date  != NULL);
  assert(p     >  0);
  
  snprintf(tname,FILENAME_MAX,"%04d/%02d/%02d/%d",date->year,date->month,date->day,p);
}

/*********************************************************************/

static FILE *open_file_r(char const *name,struct btm const *date)
{
  FILE *in;
  char  buffer[FILENAME_MAX];
  
  assert(name != NULL);
  assert(date != NULL);
  
  date_to_filename(buffer,date,name);
  in = fopen(buffer,"r");
  if (in == NULL)
    in = fopen("/dev/null","r");
    
  /*----------------------------------------------------------------------
  ; because the code was written using a different IO model (that is, if
  ; there's no data in the file to begin with, we automatically end up in
  ; EOF mode, let's trigger EOF.  Sigh.
  ;-----------------------------------------------------------------------*/
  
  ungetc(fgetc(in),in);
  return in;
}

/**********************************************************************/

static FILE *open_file_w(char const *name,struct btm const *date)
{
  FILE *out;
  char  buffer[FILENAME_MAX];
  
  assert(name != NULL);
  assert(date != NULL);
  
  date_to_filename(buffer,date,name);
  out = fopen(buffer,"w");
  return out;
}

/*********************************************************************/

static bool date_check(struct btm const *date)
{
  char tname[FILENAME_MAX];
  struct stat status;
  
  assert(date != NULL);
  date_to_dir(tname,date);
  return stat(tname,&status) == 0;
}

/************************************************************************/

static int date_checkcreate(struct btm const *date)
{
  int         rc;
  char        tname[FILENAME_MAX];
  struct stat status;
  
  assert(date != NULL);
  
  snprintf(tname,sizeof(tname),"%04d",date->year);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT)
      return errno;
    rc = mkdir(tname,0777);
    if (rc != 0)
      return errno;
  }
  
  snprintf(tname,sizeof(tname),"%04d/%02d",date->year,date->month);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT)
      return errno;
    rc = mkdir(tname,0777);
    if (rc != 0)
      return errno;
  }
  
  snprintf(tname,sizeof(tname),"%04d/%02d/%02d",date->year,date->month,date->day);
  rc = stat(tname,&status);
  if (rc != 0)
  {
    if (errno != ENOENT)
      return errno;
    rc = mkdir(tname,0777);
    if (rc != 0)
      return errno;
  }
  
  return 0;
}

/********************************************************************/

static char *blog_meta_entry(char const *name,struct btm const *date)
{
  FILE         *fp;
  char         *text;
  size_t        size;
  unsigned int  num;
  
  assert(name != NULL);
  assert(date != NULL);
  
  text = NULL;
  size = 0;
  num  = date->part;
  fp   = open_file_r(name,date);
  
  while(!feof(fp) && (num--))
  {
    ssize_t bytes = getline(&text,&size,fp);
    if (bytes == -1)
    {
      fclose(fp);
      free(text);
      return strdup("");
    }
  }
  
  fclose(fp);
  
  if (text)
  {
    char *nl = strchr(text,'\n');
    if (nl) *nl = '\0';
  }
  else
    return strdup("");
    
  return text;
}

/********************************************************************/

static size_t blog_meta_read(
        char             **plines[],
        char       const  *name,
        struct btm const  *date
)
{
  char   **lines;
  size_t   i;
  FILE    *fp;
  
  assert(plines != NULL);
  assert(name   != NULL);
  assert(date   != NULL);
  
  lines = calloc(ENTRY_MAX,sizeof(char *));
  if (lines == NULL)
  {
    *plines = NULL;
    return 0;
  }
  
  fp = open_file_r(name,date);
  if (feof(fp))
  {
    fclose(fp);
    *plines = lines;
    return 0;
  }
  
  for(i = 0 ; i < ENTRY_MAX ; i++)
  {
    char    *nl;
    ssize_t  bytes;
    size_t   size;
    
    lines[i] = NULL;
    size     = 0;
    bytes     = getline(&lines[i],&size,fp);
    if (bytes == -1)
    {
      free(lines[i]);
      break;
    }
    nl = strchr(lines[i],'\n');
    if (nl) *nl = '\0';
  }
  
  fclose(fp);
  *plines = lines;
  return i;
}

/************************************************************************/

static void blog_meta_adjust(char **plines[],size_t num,size_t maxnum)
{
  assert(plines  != NULL);
  assert(*plines != NULL);
  assert(num     <= maxnum);
  assert(num     <= ENTRY_MAX);
  assert(maxnum  <= ENTRY_MAX);
  
  char **lines = *plines;
  
  while(num < maxnum)
  {
    assert(lines[num] == NULL);
    lines[num++] = strdup("");
  }
}

/************************************************************************/

static int blog_meta_write(
        char       const *name,
        struct btm const *date,
        char             *list[],
        size_t             num
)
{
  assert(name != NULL);
  assert(date != NULL);
  assert(list != NULL);
  
  FILE *fp = open_file_w(name,date);
  if (fp == NULL)
    return errno;
    
  for (size_t i = 0 ; i < num ; i++)
    fprintf(fp,"%s\n",list[i]);
    
  fclose(fp);
  return 0;
}

/************************************************************************/

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

static size_t confL_totemplates(lua_State *L,int idx,template__t *ptemps[])
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

static size_t confL_toaffiliates(lua_State *L,int idx,aflink__t *paffs[])
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
  
  struct config *config = lua_touserdata(L,1);
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

static bool config_read(char const *conf,Blog *blog)
{
  int rc;
  
  assert(blog != NULL);
  
  if (conf == NULL)
  {
    conf = getenv("REDIRECT_BLOG_CONFIG");
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
  
  blog->config.L = luaL_newstate();
  if (blog->config.L == NULL)
  {
    syslog(LOG_ERR,"cannot create Lua state");
    return false;
  }
  
  luaL_openlibs(blog->config.L);
  
  rc = luaL_loadfile(blog->config.L,conf);
  if (rc != LUA_OK)
  {
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,lua_tostring(blog->config.L,-1));
    return false;
  }
  
  rc = lua_pcall(blog->config.L,0,0,0);
  if (rc != LUA_OK)
  {
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,lua_tostring(blog->config.L,-1));
    return false;
  }
  
  /*------------------------------------------------------------------------
  ; Ensure that searches are using the C locale, because we give the config
  ; file the full access to Lua, including os.setlocale().
  ;-------------------------------------------------------------------------*/
  
  setlocale(LC_COLLATE,"C");
  
  /*-----------------------------------------------------------------------
  ; Some items aren't optional, so let Lua do the error checking for us and
  ; return an error, which is caught here; otherwise, things might blow up.
  ; Besides, catching common configuration errors is always a good thing.
  ;------------------------------------------------------------------------*/
  
  lua_pushcfunction(blog->config.L,confL_config);
  lua_pushlightuserdata(blog->config.L,&blog->config);
  
  rc = lua_pcall(blog->config.L,1,0,0);
  if (rc != LUA_OK)
  {
    syslog(LOG_ERR,"Lua error: (%d) %s",rc,lua_tostring(blog->config.L,-1));
    return false;
  }
  
  return true;
}

/***************************************************************************/

Blog *BlogNew(char const *configfile)
{
  struct tm *ptm;
  Blog      *blog = calloc(1,sizeof(struct blog));
  
  if (blog == NULL)
  {
    syslog(LOG_ERR,"Can't allocate blog memory");
    return NULL;
  }
  
  if (!config_read(configfile,blog))
  {
    BlogFree(blog);
    return NULL;
  }
  
  umask(022);
  if (chdir(blog->config.basedir) != 0)
  {
    syslog(LOG_ERR,"%s: %s",blog->config.basedir,strerror(errno));
    BlogFree(blog);
    return NULL;
  }
  
  blog->tnow      = time(NULL);
  blog->lastmod   = 0;
  ptm             = localtime(&blog->tnow);
  blog->now.year  = ptm->tm_year + 1900;
  blog->now.month = ptm->tm_mon  + 1;
  blog->now.day   = ptm->tm_mday;
  blog->now.part  = 1;
  
  if (!set_date(".first",&blog->first,&blog->now) || !set_date(".last",&blog->last,&blog->now))
  {
    BlogFree(blog);
    return NULL;
  }
  
  if (
          (blog->last.year  == blog->now.year)
       && (blog->last.month == blog->now.month)
       && (blog->last.day   == blog->now.day)
     )
  {
    blog->now.part = blog->last.part;
  }
  
  return blog;
}

/***********************************************************************/

void BlogFree(Blog *blog)
{
  assert(blog != NULL);
  
  if (blog->config.L != NULL)
    lua_close(blog->config.L);
  free(blog);
}

/************************************************************************/

BlogEntry *BlogEntryNew(Blog *blog)
{
  assert(blog != NULL);
  
  BlogEntry *pbe = malloc(sizeof(struct blogentry));
  if (pbe != NULL)
  {
    pbe->node.ln_Succ = NULL;
    pbe->node.ln_Pred = NULL;
    pbe->blog         = blog;
    pbe->valid        = true;
    pbe->timestamp    = time(NULL);
    pbe->when.year    = blog->now.year;
    pbe->when.month   = blog->now.month;
    pbe->when.day     = blog->now.day;
    pbe->when.part    = 0;
    pbe->title        = NULL;
    pbe->class        = NULL;
    pbe->author       = NULL;
    pbe->status       = NULL;
    pbe->adtag        = NULL;
    pbe->body         = NULL;
  }
  
  return pbe;
}

/***********************************************************************/

BlogEntry *BlogEntryRead(Blog *blog,struct btm const *which)
{
  BlogEntry   *entry;
  char         pname[FILENAME_MAX];
  FILE        *sinbody;
  struct stat  status;
  
  assert(blog                             != NULL);
  assert(which                            != NULL);
  assert(which->part                      >  0);
  assert(btm_cmp_date(which,&blog->first) >= 0);
  
  if (!date_check(which))
    return NULL;
    
  date_to_part(pname,which,which->part);
  if (access(pname,R_OK) != 0)
    return NULL;
    
  entry = malloc(sizeof(struct blogentry));
  if (entry == NULL)
    return NULL;
    
  entry->node.ln_Succ = NULL;
  entry->node.ln_Pred = NULL;
  entry->valid        = true;
  entry->blog         = blog;
  entry->when.year    = which->year;
  entry->when.month   = which->month;
  entry->when.day     = which->day;
  entry->when.part    = which->part;
  entry->title        = blog_meta_entry("titles",which);
  entry->class        = blog_meta_entry("class",which);
  entry->author       = blog_meta_entry("authors",which);
  entry->status       = blog_meta_entry("status",which);
  entry->adtag        = blog_meta_entry("adtag",which);
  
  if (stat(pname,&status) == 0)
    entry->timestamp = status.st_mtime;
  else
    entry->timestamp = blog->tnow;
    
  sinbody = fopen(pname,"r");
  if (sinbody == NULL)
    entry->body = strdup("");
  else
  {
    entry->body = malloc(status.st_size + 1);
    fread(entry->body,1,status.st_size,sinbody);
    fclose(sinbody);
    entry->body[status.st_size] = '\0';
  }
  
  return entry;
}

/**********************************************************************/

void BlogEntryReadBetweenU(
        Blog             *blog,
        List             *list,
        struct btm const *restrict start,
        struct btm const *restrict end
)
{
  BlogEntry  *entry;
  struct btm  current;
  
  assert(blog  != NULL);
  assert(list  != NULL);
  assert(start != NULL);
  assert(end   != NULL);
  
  current = *start;
  
  while(btm_cmp(&current,end) <= 0)
  {
    entry = BlogEntryRead(blog,&current);
    if (entry != NULL)
    {
      if (entry->timestamp > blog->lastmod)
        blog->lastmod = entry->timestamp;
      ListAddTail(list,&entry->node);
      current.part++;
    }
    else
    {
      current.part = 1;
      btm_inc_day(&current);
    }
  }
}

/************************************************************************/

void BlogEntryReadBetweenD(
        Blog             *blog,
        List             *listb,
        struct btm const *restrict end,
        struct btm const *restrict start
)
{
  List  lista;
  
  assert(blog  != NULL);
  assert(listb != NULL);
  assert(start != NULL);
  assert(end   != NULL);
  
  ListInit(&lista);
  
  /*----------------------------------------------
  ; we read in the entries from earlier to later,
  ; then just reverse the list.
  ;----------------------------------------------*/
  
  BlogEntryReadBetweenU(blog,&lista,start,end);
  
  for
  (
    Node *node = ListRemHead(&lista);
    NodeValid(node);
    node = ListRemHead(&lista)
  )
  {
    ListAddHead(listb,node);
  }
}

/*******************************************************************/

void BlogEntryReadXD(
        Blog             *blog,
        List             *list,
        struct btm const *start,
        size_t            num
)
{
  struct btm current;
  
  assert(blog  != NULL);
  assert(list  != NULL);
  assert(start != NULL);
  assert(num   >  0);
  
  current = *start;
  
  while(num)
  {
    if (btm_cmp_date(&current,&blog->first) < 0)
      return;
      
    BlogEntry *entry = BlogEntryRead(blog,&current);
    if (entry != NULL)
    {
      if (entry->timestamp > blog->lastmod)
        blog->lastmod = entry->timestamp;
      ListAddTail(list,&entry->node);
      num--;
    }
    current.part--;
    if (current.part == 0)
    {
      current.part = ENTRY_MAX;
      btm_dec_day(&current);
    }
  }
}

/*******************************************************************/

void BlogEntryReadXU(
        Blog             *blog,
        List             *list,
        struct btm const *start,
        size_t            num
)
{
  struct btm current;
  
  assert(blog != NULL);
  assert(list != NULL);
  assert(num  >  0);
  
  current = *start;
  
  while((num) && (btm_cmp_date(&current,&blog->now) <= 0))
  {
    BlogEntry *entry = BlogEntryRead(blog,&current);
    if (entry != NULL)
    {
      if (entry->timestamp > blog->lastmod)
        blog->lastmod = entry->timestamp;
      ListAddTail(list,&entry->node);
      num--;
      current.part++;
    }
    else
    {
      current.part = 1;
      btm_inc_day(&current);
    }
  }
}

/**************************************************************************/

int BlogEntryWrite(BlogEntry *entry)
{
  char   **authors;
  char   **class;
  char   **status;
  char   **titles;
  char   **adtag;
  size_t   numa;
  size_t   numc;
  size_t   nums;
  size_t   numt;
  size_t   numad;
  size_t   maxnum;
  char     filename[FILENAME_MAX];
  FILE    *out;
  int      rc;
  int      lock;
  
  assert(entry != NULL);
  assert(entry->valid);
  
  rc = date_checkcreate(&entry->when);
  if (rc != 0)
    return rc;
    
  /*---------------------------------------------------------------------
  ; The meta-data for the entries are stored in separate files.  When
  ; updating an entry (or adding an entry), we need to rewrite these
  ; metafiles.  So, we read them all in, make the adjustments required and
  ; write them out.
  ;------------------------------------------------------------------------*/
  
  numa   = blog_meta_read(&authors,"authors",&entry->when);
  numc   = blog_meta_read(&class,  "class",  &entry->when);
  nums   = blog_meta_read(&status, "status", &entry->when);
  numt   = blog_meta_read(&titles, "titles", &entry->when);
  numad  = blog_meta_read(&adtag,  "adtag",  &entry->when);
  maxnum = max(numa,max(numc,max(nums,numt)));
  
  blog_meta_adjust(&authors,numa, maxnum);
  blog_meta_adjust(&class,  numc, maxnum);
  blog_meta_adjust(&status, nums, maxnum);
  blog_meta_adjust(&titles, numt, maxnum);
  blog_meta_adjust(&adtag,  numad,maxnum);
  
  if (entry->when.part == 0)
  {
    if (maxnum == 99)
      return ENOMEM;
      
    authors[maxnum]  = strdup(entry->author);
    class  [maxnum]  = strdup(entry->class);
    status [maxnum]  = strdup(entry->status);
    titles [maxnum]  = strdup(entry->title);
    adtag  [maxnum]  = strdup(entry->adtag);
    entry->when.part = ++maxnum;
  }
  else
  {
    free(authors[entry->when.part]);
    free(class  [entry->when.part]);
    free(status [entry->when.part]);
    free(titles [entry->when.part]);
    free(adtag  [entry->when.part]);
    
    authors[entry->when.part] = strdup(entry->author);
    class  [entry->when.part] = strdup(entry->class);
    status [entry->when.part] = strdup(entry->status);
    titles [entry->when.part] = strdup(entry->title);
    adtag  [entry->when.part] = strdup(entry->adtag);
  }
  
  blog_meta_write("authors",&entry->when,authors,maxnum);
  blog_meta_write("class"  ,&entry->when,class,  maxnum);
  blog_meta_write("status" ,&entry->when,status, maxnum);
  blog_meta_write("titles" ,&entry->when,titles, maxnum);
  blog_meta_write("adtag"  ,&entry->when,adtag,  maxnum);
  
  lock = blog_lock(entry->blog->config.lockfile);
  
  /*-------------------------------
  ; update the actual entry body
  ;---------------------------------*/
  
  date_to_part(filename,&entry->when,entry->when.part);
  out = fopen(filename,"w");
  fputs(entry->body,out);
  fclose(out);
  
  /*-----------------
  ; clean up
  ;------------------*/
  
  for(size_t i = 0 ; i < maxnum ; i++)
  {
    free(authors[i]);
    free(class[i]);
    free(status[i]);
    free(titles[i]);
    free(adtag[i]);
  }
  
  free(authors);
  free(class);
  free(status);
  free(titles);
  free(adtag);
  
  /*------------------------------------------------------------------------
  ; Oh, and if this is the latest entry to be added, update the .last file
  ; to reflect that.
  ;------------------------------------------------------------------------*/
  
  if (btm_cmp(&entry->when,&entry->blog->last) > 0)
  {
    Blog *blog = (Blog *)entry->blog; /* XXX how to handle */
    blog->last = entry->when;
    blog->now  = entry->when;
    
    out = fopen(".last","w");
    
    if (out)
    {
      fprintf(
                out,
                "%d/%02d/%02d.%d\n",
                entry->when.year,
                entry->when.month,
                entry->when.day,
                entry->when.part
        );
      fclose(out);
    }
  }
  
  entry->blog->lastmod = entry->timestamp;
  blog_unlock(entry->blog->config.lockfile,lock);
  return 0;
}

/***********************************************************************/

size_t BlogLastEntry(Blog *blog,struct btm const *when)
{
  (void)blog;
  assert(when != NULL);
  
  char name[FILENAME_MAX];
  
  for (int i = 1 ; i < ENTRY_MAX ; i++)
  {
    date_to_part(name,when,i);
    if (access(name,R_OK) == -1)
      return (size_t)i-1;
  }
  return 0;
}

/***********************************************************************/

int BlogEntryFree(BlogEntry *entry)
{
  assert(entry != NULL);
  
  if (!entry->valid)
  {
    syslog(LOG_ERR,"invalid entry being freed");
    return 0;
  }
  
  free(entry->body);
  free(entry->adtag);
  free(entry->status);
  free(entry->author);
  free(entry->class);
  free(entry->title);
  free(entry);
  return 0;
}

/***********************************************************************/
