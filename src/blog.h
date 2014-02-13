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
  char             *lockfile;
  int               lock;
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

extern BlogEntry	BlogEntryNew		(Blog);
extern BlogEntry	BlogEntryRead		(Blog,struct btm *);
extern void		BlogEntryReadBetweenU	(Blog,List *,struct btm *,struct btm *);
extern void		BlogEntryReadBetweenD	(Blog,List *,struct btm *,struct btm *);
extern void		BlogEntryReadXD		(Blog,List *,struct btm *,size_t);
extern void		BlogEntryReadXU		(Blog,List *,struct btm *,size_t);
extern int		BlogEntryWrite		(BlogEntry);
extern int		BlogEntryFree		(BlogEntry);

/**********************************************************************/

#endif

