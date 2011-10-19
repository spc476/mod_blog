


/************************************************************************/

int page_generation(Request req __attribute__((unused)))
{
  lua_getglobal(g_L,"templates");
  max = lua_objlen(g_L,-1);
  
  for (i = 1 ; i <= max ; i++)
  {
    lua_pushinteger(g_L,i);
    lua_gettable(g_L,-2);
    
    lua_getfield(g_L,-1,"template");
    lua_getfield(g_L,-2,"reverse");
    lua_getfield(g_L,-3,"fullurl");
    lua_getfield(g_L,-4,"output");
    lua_getfield(g_L,-5,"items");
    
    template.template = lua_tostring (L,-5);
    template.reverse  = lua_toboolean(L,-4);
    template.fullurl  = lua_toboolean(L,-3);
    
    out = fopen(lua_tostring(L,-2),"w");
    if (out == NULL)
    {
      syslog(LOG_ERR,"%s: %s",lua_tostring(L,-2),strerror(errno));
      lua_pop(L,6);
      continue;
    }
    
    if (lua_isstring(L,-1))
    {
      const char *x;
      char       *p;
      
      x = lua_tostring(L,-1);
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
    else if (lua_isnumber(L,-1))
    {
      template.items   = lua_tointeger(L,-2);
      template.pagegen = pagegen_items;
    }
    else
    {
      syslog(LOG_ERR,"wrong type for items");
      lua_pop(L,6);
      continue;
    }
    
    (*template.pagegen)(&template,out,&gd.now);
    fclose(out);
    lua_pop(L,6);
  }
  
  lua_pop(L,1);
  return 0;
}

/*************************************************************************/

int pagegen_items(
	const template__t *const restrict template,
	FILE              *const restrict out,
	const struct btm  *const restrict when
)
{
  struct btm       thisday;
  char            *tags;
  struct callback  cbd;
  
  assert(template != NULL);
  assert(out      != NULL);
  assert(when     != NULL);
  
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
