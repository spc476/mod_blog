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
  int          year;
  unsigned int month;
  unsigned int day;
  unsigned int part;
};

/***********************************************************/

unsigned int	 max_monthday	(int,unsigned int);
void		 btm_add_day	(struct btm *);
void		 btm_sub_part	(struct btm *);
void		 btm_sub_day	(struct btm *);
void		 btm_add_month	(struct btm *);
void		 btm_sub_month	(struct btm *);
int		 btm_cmp	(const struct btm *,const struct btm *);
int		 btm_cmp_date	(const struct btm *,const struct btm *);
void		 tm_init	(struct tm *);

#endif

