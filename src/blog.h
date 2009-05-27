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

#include <time.h>

#include <cgilib/nodelist.h>
#include <cgilib/errors.h>

/*******************************************************************/

typedef struct blogentry
{
  Node        node;
  char       *date;
  int         number;
  const char *title;
  const char *class;
  const char *author;
  size_t      bsize;
  void       *body;
} *BlogEntry;

typedef struct blogday
{
  Node       node;
  char      *date;
  struct tm  tm_date;
  int        number;
  int        curnum;		/* hack to avoid making a structure */
  int        stentry;		/* hack to avoid making a structure */
  int        endentry;		/* hack to avoid making a structure */
  BlogEntry  entries[100];	/* hack I know ... get Mark's varray stuff */
} *BlogDay;

/*********************************************************************/

int		(BlogInit)		(void);
int		(BlogLock)		(const char *);
int		(BlogUnlock)		(int);
int		(BlogDayRead)		(BlogDay *,struct tm *);
int		(BlogDayWrite)		(BlogDay);
int		(BlogDayEntryAdd)	(BlogDay,BlogEntry);
int		(BlogDayEntryRemove)	(BlogDay,int);
int		(BlogDayFree)		(BlogDay *);

int		(BlogEntryNew)		(BlogEntry *,const char *,const char *,const char *,void *,size_t);
int		(BlogEntryRead)		(BlogEntry);
int		(BlogEntryWrite)	(BlogEntry);
int		(BlogEntryFlush)	(BlogEntry);
int		(BlogEntryFree)		(BlogEntry *);

/**********************************************************************/

#endif
