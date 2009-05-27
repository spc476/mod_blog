
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
    extern const char   *const g_scriptname;
#endif
extern const char *const   g_name;
extern const char *const   g_basedir;
extern const char *const   g_webdir;
extern const char *const   g_baseurl;
extern const char *const   g_fullbaseurl;
extern const char *        g_templates;		/* work on */
extern const char *const   g_rsstemplates;
extern const char *const   g_daypage;
extern const int           g_days;
extern const char *const   g_rssfile;
extern const int           g_rssitems;
extern const int           g_rssreverse;
extern const int           g_styear;
extern const int           g_stmon;
extern const int           g_stday;
extern const char *const   g_author;
extern const char *const   g_email;
extern const char *const   g_authorfile;
extern const char *const   g_updatetype;
extern const char *const   g_lockfile;
extern const int           g_weblogcom;
extern const char *const   g_weblogcomurl;
extern int                 g_emailupdate;	/* work on */
extern const char         *g_emaildb;
extern const char *const   g_emailsubject;
extern const char *const   g_emailmsg;
extern const char *const   g_tabtemplates;
extern const char *const   g_tabfile;
extern const int           g_tabreverse;
extern const int           g_tzhour;
extern const int           g_tzmin;
extern const char *const   g_backend;
extern       void        (*g_conversion)(char *,Buffer,Buffer);
extern volatile int        g_debug;

int		GlobalsInit		(char *);
void		set_g_updatetype	(char *);
void		set_g_emailupdate	(char *);
void		set_g_conversion	(char *);

int		main_cgi_get		(Cgi,int,char *[]);
int		main_cgi_post		(Cgi,int,char *[]);
int		main_cli		(int,char *[]);

#endif

