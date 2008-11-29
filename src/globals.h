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

#include <cgilib/cgi.h>

#include "frontend.h"
#include "timeutil.h"
#include "blog.h"

/*****************************************************************
*
* Global variables---some defaults are defined in "globals.c" but
* these can be overwritten (or defined for those without a default)
* by the configuration file.  If any are added here, add them to
* "globals.c" and add don't forget to update GlobalsInit() or
* SystemLimit() as appropriate.
*
*******************************************************************/

#ifdef PARALLEL_HACK
    extern const char   *const c_scriptname;
#endif
extern const char *const      c_name;
extern const char *const      c_basedir;
extern const char *const      c_webdir;
extern const char *const      c_baseurl;
extern const char *const      c_fullbaseurl;
extern const char *const      c_rsstemplates;
extern const char *const      c_atomtemplates;
extern const char *const      c_daypage;
extern const int              c_days;
extern const char *const      c_rssfile;
extern const char *const      c_atomfile;
extern const int              c_rssitems;
extern const int              cf_rssreverse;
extern const int              c_styear;
extern const int              c_stmon;
extern const int              c_stday;
extern const char *const      c_author;
extern const char *const      c_email;
extern const char *const      c_authorfile;
extern const char *const      c_updatetype;
extern const char *const      c_lockfile;
extern const int              cf_weblogcom;
extern const char *const      c_weblogcomurl;
extern const char            *c_emaildb;
extern const char *const      c_emailsubject;
extern const char *const      c_emailmsg;
extern const char *const      c_tabtemplates;
extern const char *const      c_tabfile;
extern const int              cf_tabreverse;
extern const int              c_tzhour;
extern const int              c_tzmin;
extern       void           (*c_conversion)(char *,Stream,Stream);
extern const struct btm       c_start;
extern const struct btm       c_now;
extern const char *const      c_overview;

extern const char *           g_templates;	/* work on */
extern int                    gf_emailupdate;	/* work on */
extern volatile int           gf_debug;
extern Blog                   g_blog;
extern struct display         gd;		/* work on */

int		GlobalsInit		(char *);
void		set_c_updatetype	(char *);
void		set_gf_emailupdate	(char *);
void		set_c_conversion	(char *);
void		set_time		(void);

int		main_cgi_head		(Cgi,int,char *[]);
int		main_cgi_get		(Cgi,int,char *[]);
int		main_cgi_post		(Cgi,int,char *[]);
int		main_cli		(int,char *[]);

#endif

