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

struct btm
{
  int year;
  int month;
  int day;
  int part;
};

/***********************************************************/

extern int	max_monthday	(const int,const int);
extern void	btm_add_day	(struct btm *const);
extern void	btm_sub_part	(struct btm *const);
extern void	btm_sub_day	(struct btm *const);
extern void	btm_add_month	(struct btm *const);
extern void	btm_sub_month	(struct btm *const);
extern int	btm_cmp		(const struct btm *const restrict,const struct btm *const restrict);
extern int	btm_cmp_date	(const struct btm *const restrict,const struct btm *const restrict);
extern void	tm_init		(struct tm *const);

#endif

