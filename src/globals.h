/**************************************************************
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
****************************************************************/

#ifndef I_82F50023_10E0_5FB8_A706_318F243A6012
#define I_82F50023_10E0_5FB8_A706_318F243A6012

#include <stdbool.h>
#include <cgilib6/cgi.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "frontend.h"
#include "backend.h"
#include "timeutil.h"
#include "blog.h"
#include "config.h"

#define GENERATOR       "mod_blog" " " PROG_VERSION

/***************************************************************************
*
* Global variables---some defaults are defined in "globals.c" but these can
* be overwritten (or defined for those without a default) by the
* configuration file.  If any are added here, add them to "globals.c" and
* add don't forget to update GlobalsInit()
*
****************************************************************************/

extern char const *const c_updatetype;  /* work on */
extern char const *const c_baseurl;     /* work on */
extern char const *const c_fullbaseurl; /* work on */

extern char const     *g_templates;     /* work on */
extern bool volatile   gf_debug;        /* remove? */
extern Blog           *g_blog;
extern config__s      *g_config;        /* work on */
extern struct display  gd;              /* work on */

extern int  GlobalsInit        (char const *);
extern void set_c_updatetype   (char const *);
extern void set_cf_emailupdate (char const *);
extern void set_c_conversion   (char const *);
extern int  main_cgi_get       (Cgi);
extern int  main_cgi_post      (Cgi);
extern int  main_cli           (int,char *[]);

#endif
