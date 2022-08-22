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

#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>

#include <syslog.h>
#include <cgilib6/url.h>
#include <cgilib6/util.h>

#include "conversion.h"
#include "frontend.h"

/************************************************************/

config__s      *c_config;
Blog           *g_blog;
struct display  gd;

/****************************************************/

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
  if (g_blog   != NULL) BlogFree(g_blog);
  if (c_config != NULL) config_free(c_config);
  
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
  free(gd.fullbaseurl);
  free(gd.baseurl);
}

/***********************************************************************/

static void set_url(char const *turl)
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
  ;-----------------------------------------------------------------------*/
  
  len = strlen(fbu);
  if (len) len--;
  if (fbu[len] == '/') fbu[len] = '\0';
  
  len = strlen(bu);
  if (len) len--;
  if (bu[len] == '/') bu[len] = '\0';
  
  gd.fullbaseurl = fbu;
  gd.baseurl     = bu;
  UrlFree(url);
}

/***************************************************************************/

void set_cf_emailupdate(char const *value)
{
  if (!emptynull_string(value))
  {
    if (strcmp(value,"no") == 0)
      c_config->email.notify = false;
    else if (strcmp(value,"yes") == 0)
      c_config->email.notify = true;
  }
}

/***************************************************************************/

int GlobalsInit(char const *conf)
{
  atexit(globals_free);
  seed_rng();
  c_config = config_lua(conf);
  if (c_config == NULL)
    return ENOMEM;
    
  g_blog = BlogNew(c_config->basedir,c_config->lockfile);
  if (g_blog == NULL)
    return ENOMEM;
    
  set_url(c_config->url);
  gd.template       = c_config->templates[0].template; /* XXX hack fix */
  gd.f.navprev      = true;
  gd.f.navnext      = true;
  
  /*-------------------------------------------------------
  ; for most sorting routines, I just assume C sorting
  ; conventions---this makes sure I have those for sorting
  ; and searching only.
  ;--------------------------------------------------------*/
  
  setlocale(LC_COLLATE,"C");
  return 0;
}

/********************************************************************/
