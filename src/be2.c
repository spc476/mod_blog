
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <syslog.h>

#include "conf.h"
#include "blog.h"
#include "frontend.h"
#include "backend.h"
#include "fix.h"
#include "globals.h"

char *tag_collect (List *);
char *tag_pick (const char *);
void  free_entries (List *);

/************************************************************************/

int page_generation(Request req __attribute__((unused)))
{
  size_t max;
  size_t i;
  
  syslog(LOG_DEBUG,"generating pages");
  lua_getglobal(g_L,"templates");
  max = lua_objlen(g_L,-1);
  
  for (i = 1 ; i <= max ; i++)
  {
    template__t  template;
    FILE        *out;
    
    lua_pushinteger(g_L,i);
    lua_gettable(g_L,-2);
    
    lua_getfield(g_L,-1,"template");
    lua_getfield(g_L,-2,"reverse");
    lua_getfield(g_L,-3,"fullurl");
    lua_getfield(g_L,-4,"output");
    lua_getfield(g_L,-5,"items");
    
    template.template = lua_tostring (g_L,-5);
    template.reverse  = lua_toboolean(g_L,-4);
    template.fullurl  = lua_toboolean(g_L,-3);
    
    out = fopen(lua_tostring(g_L,-2),"w");
    if (out == NULL)
    {
      syslog(LOG_ERR,"%s: %s",lua_tostring(g_L,-2),strerror(errno));
      lua_pop(g_L,6);
      continue;
    }
    
    if (lua_isstring(g_L,-1))
    {
      const char *x;
      char       *p;
      
      x = lua_tostring(g_L,-1);
      template.items = strtoul(x,&p,10);
      switch(*p)
      {
        case 'd': break;
        case 'w': template.items *=  7; break;
        case 'm': template.items *= 30; break;
        default: break;
      }
      template.pagegen = pagegen_days;
    }
    else if (lua_isnumber(g_L,-1))
    {
      template.items   = lua_tointeger(g_L,-1);
      template.pagegen = pagegen_items;
    }
    else
    {
      syslog(LOG_ERR,"wrong type for items");
      lua_pop(g_L,6);
      continue;
    }
    
    (*template.pagegen)(&template,out,&gd.now);
    fclose(out);
    lua_pop(g_L,6);
  }
  
  lua_pop(g_L,1);
  return 0;
}

/*************************************************************************/

int pagegen_items(
	const template__t *const restrict template,
	FILE              *const restrict out,
	const struct btm  *const restrict when
)
{
  struct btm            thisday;
  char                 *tags;
  struct callback_data  cbd;
  
  assert(template != NULL);
  assert(out      != NULL);
  assert(when     != NULL);
  
  syslog(LOG_DEBUG,"items template %s(%lu)",template->template,(unsigned long)template->items);
  g_templates  = template->template;
  gd.f.fullurl = template->fullurl;
  gd.f.reverse = template->reverse;
  thisday      = *when;
  
  memset(&cbd,0,sizeof(struct callback_data));
  
  ListInit(&cbd.list);
  if (template->reverse)
    BlogEntryReadXD(g_blog,&cbd.list,&thisday,template->items);
  else
    BlogEntryReadXU(g_blog,&cbd.list,&thisday,template->items);
  
  tags     = tag_collect(&cbd.list);
  gd.adtag = tag_pick(tags);
  free(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);
  return 0;
}

/************************************************************************/

int pagegen_days(
	const template__t *const restrict template,
	FILE              *const restrict out,
	const struct btm  *const restrict when
)
{
  struct btm            thisday;
  size_t                days;
  char                 *tags;
  struct callback_data  cbd;
  bool                  added;
  
  assert(template != NULL);
  assert(out      != NULL);
  assert(when     != NULL);

  syslog(LOG_DEBUG,"days template %s(%lu)",template->template,(unsigned long)template->items);  
  g_templates  = template->template;
  gd.f.fullurl = false;
  gd.f.reverse = true;
  thisday      = *when;
  
  memset(&cbd,0,sizeof(struct callback_data));
  
  for (ListInit(&cbd.list) , days = 0 , added = false ; days < template->items ; )
  {
    BlogEntry entry;
    
    if (btm_cmp(&thisday,&gd.begin) < 0) break;
    
    entry = BlogEntryRead(g_blog,&thisday);
    if (entry)
    {
      ListAddTail(&cbd.list,&entry->node);
      added = true;
    }
    
    thisday.part--;
    if (thisday.part == 0)
    {
      thisday.part = 23;
      btm_sub_day(&thisday);
      if (added)
        days++;
      added = false;
    }
  }
  
  tags = tag_collect(&cbd.list);
  gd.adtag = tag_pick(tags);
  free(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);
  return 0;
}

/************************************************************************/
