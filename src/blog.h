/*******************************************************************
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
********************************************************************/

#ifndef I_A7907483_71BF_5594_9AA6_58C785CB9FFA
#define I_A7907483_71BF_5594_9AA6_58C785CB9FFA

#include <stdbool.h>
#include <time.h>

#include <lua.h>
#include <cgilib6/nodelist.h>

#include "timeutil.h"

/*******************************************************************/

struct fields
{
  size_t uid;
  size_t name;
  size_t email;
};

struct author
{
  char const    *name;
  char const    *email;
  char const    *file;
  struct fields  fields;
};

struct bemail
{
  char const *list;
  char const *message;
  char const *subject;
  bool        notify; /* derived */
};

typedef struct template
{
  char const *template;
  char const *file;
  char const *posthook;
  char const *pagegen;
  size_t      items;
  bool        reverse;
  bool        fullurl;
} template__t;

typedef struct aflink
{
  char const *proto;
  size_t      psize;
  char const *format;
} aflink__t;

struct config
{
  char const    *name;
  char const    *description;
  char const    *class;
  char const    *basedir;
  char const    *lockfile;
  char const    *webdir;
  char const    *url;
  char const    *prehook;
  char const    *posthook;
  char const    *adtag;
  char const    *conversion;
  struct author  author;
  template__t   *templates;
  size_t         templatenum;
  struct bemail  email;
  aflink__t     *affiliates;
  size_t         affiliatenum;
  char const    *baseurl; /* derived from URL */
  lua_State     *L;
};

typedef struct blog
{
  struct config config;
  struct btm    first;
  struct btm    last;
  struct btm    now;
  time_t        tnow;
  time_t        lastmod;
} Blog;

typedef struct blogentry
{
  Node        node;
  bool        valid;
  Blog       *blog;
  time_t      timestamp;
  struct btm  when;
  char       *title;
  char       *class;
  char       *author;
  char       *status;
  char       *adtag;
  char       *body;
} BlogEntry;

/*********************************************************************/

extern Blog      *BlogNew               (char const *);
extern void       BlogFree              (Blog *);
extern BlogEntry *BlogEntryNew          (Blog *);
extern BlogEntry *BlogEntryRead         (Blog *,struct btm const *);
extern void       BlogEntryReadBetweenU (Blog *,List *,struct btm const *restrict,struct btm const *restrict);
extern void       BlogEntryReadBetweenD (Blog *,List *,struct btm const *restrict,struct btm const *restrict);
extern void       BlogEntryReadXD       (Blog *,List *,struct btm const *,size_t);
extern void       BlogEntryReadXU       (Blog *,List *,struct btm const *,size_t);
extern int        BlogEntryWrite        (BlogEntry *);
extern int        BlogEntryFree         (BlogEntry *);

/**********************************************************************/

#endif
