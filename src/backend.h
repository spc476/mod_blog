/***********************************************
*
* Copyright 2011 by Sean Conner.  All Rights Reserved.
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
************************************************/

#ifndef I_5B8ED10A_F8F4_5F83_A7EA_CD76EE7A05D8
#define I_5B8ED10A_F8F4_5F83_A7EA_CD76EE7A05D8

#include <stdio.h>

#include "frontend.h"
#include "blog.h"
#include "timeutil.h"
#include "wbtum.h"

typedef int (*pagegen__f)(Blog const *,Request *,struct template const *,FILE *);

struct callback_data
{
  List               list;
  BlogEntry         *entry;    /* current entry being processed */
  FILE              *ad;       /* file containing ad            */
  char              *adtag;
  char              *adcat;
  FILE              *wm;       /* file containing webmentions   */
  char              *wmtitle;  /* webmention title              */
  char              *wmurl;    /* webmention url                */
  struct btm         last;     /* timestamp of previous entry   */
  struct btm         previous;
  struct btm         next;
  unit__e            navunit;
  template__t const *template;
  Request     const *request;
  Blog        const *blog;
};

/************************************************/

extern struct callback_data *callback_init    (struct callback_data *,Blog const *,Request const *);
extern pagegen__f            TO_pagegen       (char const *);
extern int                   generate_thisday (Blog const *,Request *,FILE *,struct btm);
extern int                   generate_pages   (Blog const *,Request *);
extern int                   pagegen_items    (Blog const *,Request *,template__t const *,FILE *);
extern int                   pagegen_days     (Blog const *,Request *,template__t const *,FILE *);
extern int                   tumbler_page     (Blog const *,Request *,tumbler__s *,int (*)(Blog const *,Request *,int,char const *,...));
extern void                  generic_cb       (char const *,FILE *,void *);
extern bool                  run_hook         (char const *,char const *[]);

#endif
