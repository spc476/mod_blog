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
#include "config.h"

typedef int (*pagegen__f)(struct template const *,FILE *,Blog *);

struct callback_data
{
  List        list;
  BlogEntry  *entry;    /* current entry being processed */
  FILE       *ad;       /* file containing ad */
  char       *adtag;
  struct btm  last;     /* timestamp of previous entry */
  struct btm  previous;
  struct btm  next;
  unit__e     navunit;
};

/************************************************/

extern struct callback_data *callback_init    (struct callback_data *);
extern pagegen__f            TO_pagegen       (char const *);
extern int                   generate_thisday (FILE *,struct btm);
extern int                   generate_pages   (void);
extern int                   pagegen_items    (template__t const *,FILE *,Blog *);
extern int                   pagegen_days     (template__t const *,FILE *,Blog *);
extern int                   tumbler_page     (tumbler__s *,int (*)(Request *,int,char const *,...));
extern void                  generic_cb       (char const *,FILE *,void *);
extern bool                  run_hook         (char const *,char const **);

#endif
