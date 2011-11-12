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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>
#include <cgilib6/cgi.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../version.h"
#include "frontend.h"
#include "timeutil.h"
#include "blog.h"

#define GENERATOR	"mod_blog" " " PROG_VERSION

/*****************************************************************
*
* Global variables---some defaults are defined in "globals.c" but
* these can be overwritten (or defined for those without a default)
* by the configuration file.  If any are added here, add them to
* "globals.c" and add don't forget to update GlobalsInit() or
* SystemLimit() as appropriate.
*
*******************************************************************/

extern const char *const      c_name;
extern const char *const      c_basedir;
extern const char *const      c_webdir;
extern const char *const      c_baseurl;
extern const char *const      c_fullbaseurl;
extern const char *const      c_htmltemplates;
extern const char *const      c_daypage;
extern const int              c_days;
extern const char *const      c_author;
extern const char *const      c_email;
extern const char *const      c_authorfile;
extern const char *const      c_updatetype;
extern const char *const      c_lockfile;
extern const char            *c_emaildb;
extern const char *const      c_emailsubject;
extern const char *const      c_emailmsg;
extern const int              c_tzhour;
extern const int              c_tzmin;
extern       void           (*c_conversion)(FILE *,FILE *);
extern const struct btm       c_start;
extern const struct btm       c_now;
extern const char *const      c_overview;
extern const bool             cf_facebook;
extern const char            *c_facebook_ap_id;
extern const char            *c_facebook_ap_secret;
extern const char            *c_facebook_user;
extern const template__t     *const c_templates;
extern const size_t           c_numtemplates;

extern lua_State             *g_L;
extern const char *           g_templates;	/* work on */
extern bool                   gf_emailupdate;	/* work on */
extern volatile bool          gf_debug;
extern Blog                   g_blog;
extern struct display         gd;		/* work on */

int		GlobalsInit		(const char *const);
void		set_c_updatetype	(const char *const);
void		set_gf_emailupdate	(const char *const);
void		set_cf_facebook		(const char *const);
void		set_c_conversion	(char *const);
void		set_time		(void);

int		main_cgi_head		(Cgi,int,char *[]);
int		main_cgi_get		(Cgi,int,char *[]);
int		main_cgi_post		(Cgi,int,char *[]);
int		main_cli		(int,char *[]);

#endif

