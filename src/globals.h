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

#define GENERATOR       "mod_blog" " " PROG_VERSION

/***************************************************************************
*
* Global variables---some defaults are defined in "globals.c" but these can
* be overwritten (or defined for those without a default) by the
* configuration file.  If any are added here, add them to "globals.c" and
* add don't forget to update GlobalsInit()
*
****************************************************************************/

extern char        const  *const c_name;
extern char        const  *const c_basedir;
extern char        const  *const c_webdir;
extern char        const  *const c_author;
extern char        const  *const c_email;
extern char        const  *const c_authorfile;
extern char        const  *const c_updatetype;
extern char        const  *const c_lockfile;
extern char        const  *const c_emaildb;
extern char        const  *const c_emailsubject;
extern char        const  *const c_emailmsg;
extern char        const  *const c_overview;
extern char        const  *const c_adtag;
extern char        const  *const c_baseurl;
extern char        const  *const c_fullbaseurl;
extern char        const  *const c_htmltemplates;
extern template__t const  *const c_templates;
extern aflink__t   const  *const c_aflinks;
extern void              (*const c_conversion)(FILE *,FILE *);
extern int         const         c_days;
extern size_t      const         c_af_uid;
extern size_t      const         c_af_name;
extern int         const         c_tzhour;
extern int         const         c_tzmin;
extern bool        const         cf_emailupdate;
extern size_t      const         c_numtemplates;
extern size_t      const         c_numaflinks;

extern lua_State      *g_L;
extern char const     *g_templates;      /* work on */
extern bool volatile   gf_debug;
extern Blog            g_blog;
extern struct display  gd;               /* work on */

extern int  GlobalsInit        (char const *);
extern void set_c_updatetype   (char const *const);
extern void set_cf_emailupdate (char const *const);
extern void set_c_conversion   (char const *const);
extern void set_c_url          (char const *const);
extern void set_time           (void);
extern int  main_cgi_head      (Cgi);
extern int  main_cgi_get       (Cgi);
extern int  main_cgi_post      (Cgi);
extern int  main_cli           (int,char *[]);

#endif
