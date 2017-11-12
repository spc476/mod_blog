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

#ifndef I_F3FFF9B1_0536_542C_B5AB_DB26462A0322
#define I_F3FFF9B1_0536_542C_B5AB_DB26462A0322

#include <time.h>

struct btm
{
  int year;
  int month;
  int day;
  int part;
};

/***********************************************************/

extern int  max_monthday  (int const,int const);
extern void btm_inc_day   (struct btm *);
extern void btm_dec_part  (struct btm *);
extern void btm_dec_day   (struct btm *);
extern void btm_dec_month (struct btm *);
extern void btm_inc_month (struct btm *);
extern int  btm_cmp       (struct btm const *restrict,struct btm const *restrict);
extern int  btm_cmp_date  (struct btm const *restrict,struct btm const *restrict);
extern int  btm_cmp_month (struct btm const *restrict,struct btm const *restrict);
extern int  btm_cmp_year  (struct btm const *restrict,struct btm const *restrict);
extern void tm_init       (struct tm *);

#endif
