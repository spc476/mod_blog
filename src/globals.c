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
#include "config.h"

/************************************************************/

char const     *c_updatetype = "NewEntry"; /* no free */
char           *c_baseurl;
char           *c_fullbaseurl;

char const     *g_templates;
bool volatile   gf_debug = false;
Blog           *g_blog;
config__s      *g_config;
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

static void set_c_url(char const *turl)
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
  if (gd.cgi   != NULL) CgiFree(gd.cgi);
  if (g_blog   != NULL) BlogFree(g_blog);
  if (g_config != NULL) config_free(g_config);
  
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
  free(c_baseurl);
  free(c_fullbaseurl);
}

/***********************************************************************/

int GlobalsInit(char const *conf)
{
  atexit(globals_free);
  seed_rng();
  g_config = config_lua(conf);
  if (g_config == NULL)
    return ENOMEM;
    
  g_blog = BlogNew(g_config->basedir,g_config->lockfile);
  if (g_blog == NULL)
    return ENOMEM;
    
  set_c_url(g_config->url);
  
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
  if (!emptynull_string(value))
  {
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
}

/************************************************************************/

void set_cf_emailupdate(char const *value)
{
  if (!emptynull_string(value))
  {
    if (strcmp(value,"no") == 0)
      g_config->email.notify = false;
    else if (strcmp(value,"yes") == 0)
      g_config->email.notify = true;
  }
}

/***************************************************************************/

void set_c_conversion(char const *value)
{
  if (!emptynull_string(value))
  {
    if (strcmp(value,"text") == 0)
      g_config->conversion = text_conversion;
    else if (strcmp(value,"mixed") == 0)
      g_config->conversion = mixed_conversion;
    else if (strcmp(value,"html") == 0)
      g_config->conversion = html_conversion;
    else if (strcmp(value,"none") == 0)
      g_config->conversion = no_conversion;
  }
}

/**************************************************************************/
