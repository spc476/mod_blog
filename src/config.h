/**************************************************************
*
* Copyright 2022 by Sean Conner.  All Rights Reserved.
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

#ifndef I_B6A18852_A703_5C27_8A63_33F63C0D5D40
#define I_B6A18852_A703_5C27_8A63_33F63C0D5D40

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include "blog.h"
#include "wbtum.h"
#include "backend.h"

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
  bool        notify;
};

typedef struct
{
  char const     *name;
  char const     *description;
  char const     *class;
  char const     *basedir;
  char const     *lockfile;
  char const     *webdir;
  char const     *url;
  char const     *prehook;
  char const     *posthook;
  char const     *adtag;
  bool            debug;
  void          (*conversion)(FILE *restrict,FILE *restrict);
  struct author   author;
  template__t    *templates;
  size_t          templatenum;
  struct bemail   email;
  aflink__t      *affiliates;
  size_t          affiliatenum;
  char           *baseurl;     /* work on removing the need for this */
  char           *fullbaseurl; /* work on removing the need for this */
  void           *user;
} config__s;

extern config__s *config_lua (char const *);
extern void       config_free(config__s *);

#endif
