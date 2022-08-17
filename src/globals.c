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

#include <cgilib6/util.h>

#include "conversion.h"
#include "frontend.h"
#include "config.h"

/************************************************************/

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
    
  gd.template = g_config->templates[0].template; /* XXX hack fix */

  /*-------------------------------------------------------
  ; for most sorting routines, I just assume C sorting
  ; conventions---this makes sure I have those for sorting
  ; and searching only.
  ;--------------------------------------------------------*/
  
  setlocale(LC_COLLATE,"C");
  return 0;
}

/********************************************************************/

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
