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

#ifndef BLOG_MKENTRY
#define BLOG_MKENTRY

#include <stdbool.h>
#include <cgilib6/nodelist.h>

#include "timeutil.h"

/*******************************************************************/

typedef struct blog
{
  char       *lockfile;
  int         lock;
  struct btm  first;
  struct btm  last;
  struct btm  now;
  time_t      tnow;
} *Blog;

typedef struct blogentry
{
  Node        node;
  bool        valid;
  Blog        blog;
  time_t      timestamp;
  struct btm  when;
  char       *title;
  char       *class;
  char       *author;
  char       *status;
  char       *body;
} *BlogEntry;

/*********************************************************************/

extern Blog		BlogNew			(const char *,const char *);
extern int		BlogLock		(Blog);
extern int		BlogUnlock		(Blog);
extern int		BlogFree		(Blog);

extern BlogEntry	BlogEntryNew		(const Blog);
extern BlogEntry	BlogEntryRead		(const Blog,const struct btm *);
extern void		BlogEntryReadBetweenU	(const Blog,List *,const struct btm *restrict,const struct btm *restrict);
extern void		BlogEntryReadBetweenD	(const Blog,List *,const struct btm *restrict,const struct btm *restrict);
extern void		BlogEntryReadXD		(const Blog,List *,const struct btm *restrict,size_t);
extern void		BlogEntryReadXU		(const Blog,List *,size_t);
extern int		BlogEntryWrite		(const BlogEntry);
extern int		BlogEntryFree		(BlogEntry);

/**********************************************************************/

#endif

