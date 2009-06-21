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

#include <cgilib6/nodelist.h>
#include <cgilib6/errors.h>

#include "timeutil.h"

/*******************************************************************/

typedef struct blog
{
  char             *lockfile;
  int               lock;
  struct btm        cache;
  size_t            max;
  size_t            idx;
  struct blogentry *entries[100];
} *Blog;
  
typedef struct blogentry
{
  Node        node;
  Blog        blog;
  time_t      timestamp;
  struct btm  when;
  char       *title;
  char       *class;
  char       *author;
  char       *body;
} *BlogEntry;

/*********************************************************************/

Blog		(BlogNew)		(const char *,const char *);
int		(BlogLock)		(Blog);
int		(BlogUnlock)		(Blog);
int		(BlogFree)		(Blog);

BlogEntry	(BlogEntryNew)		(Blog);
BlogEntry	(BlogEntryRead)		(Blog,struct btm *);
void		(BlogEntryReadBetweenU)	(Blog,List *,struct btm *,struct btm *);
void		(BlogEntryReadBetweenD)	(Blog,List *,struct btm *,struct btm *);
void		(BlogEntryReadXD)	(Blog,List *,struct btm *,size_t);
void		(BlogEntryReadXU)	(Blog,List *,struct btm *,size_t);
int		(BlogEntryWrite)	(BlogEntry);
int		(BlogEntryFree)		(BlogEntry);

/**********************************************************************/

#endif

