
/********************************************
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
*********************************************/

#ifndef BLOG_UTIL
#define BLOG_UTIL

#include <time.h>

const char		*const nth		(unsigned long);
int		 max_monthday	(int,int);
void		 month_add	(struct tm *);
void		 day_add	(struct tm *);
void		 month_sub	(struct tm *);
void		 day_sub	(struct tm *);
void		 tm_init	(struct tm *);
int		 tm_cmp		(struct tm *,struct tm *);
void             tm_to_tm       (struct tm *);
void             tm_to_blog     (struct tm *);
void		 tm_fromstring	(struct tm *,char *);
long		 days_between	(struct tm *,struct tm *);

#endif

