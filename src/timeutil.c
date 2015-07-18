/************
*
* Copyright 2001-2007 by Sean Conner.  All Rights Reserved.
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
***************************/

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "timeutil.h"

/*************************************************************************/

int max_monthday(const int year,const int month)
{
  static const int days[] = { 31,0,31,30,31,30,31,31,30,31,30,31 } ;
  
  assert(year  >  0);
  assert(month >  0);
  assert(month < 13);

  if (month == 2)
  {
    /*----------------------------------------------------------------------
    ; in case you didn't know, leap years are those years that are divisible
    ; by 4, except if it's divisible by 100, then it's not, unless it's
    ; divisible by 400, then it is.  1800 and 1900 were NOT leap years, but
    ; 2000 is.
    ;----------------------------------------------------------------------*/
    
    if ((year % 400) == 0) return(29);
    if ((year % 100) == 0) return(28);
    if ((year %   4) == 0) return(29);
    return(28);
  }
  else
    return(days[month - 1]);
}

/***************************************************************************/

void btm_add_day(struct btm *const d)
{
  assert(d != NULL);
  
  d->day ++;
  if (d->day > max_monthday(d->year,d->month))
  {
    d->day = 1;
    btm_add_month(d);
  }
}

/*************************************************************************/

void btm_sub_part(struct btm *const d)
{
  assert(d != NULL);
  
  d->part--;
  if (d->part == 0)
  {
    d->part = 23;
    btm_sub_day(d);
  }
}

/************************************************************************/

void btm_sub_day(struct btm *const d)
{
  assert(d != NULL);
  d->day--;
  if (d->day == 0)
  {
    btm_sub_month(d);
    d->day = max_monthday(d->year,d->month);
  }
}

/***********************************************************************/

void btm_add_month(struct btm *const d)
{
  assert(d != NULL);
  
  d->month++;
  if (d->month == 13)
  {
    d->month = 1;
    d->year++;
  }
}

/***********************************************************************/

void btm_sub_month(struct btm *const d)
{
  assert(d != NULL);
  
  d->month--;
  if (d->month == 0)
  {
    d->month = 12;
    d->year--;
  }
}

/***********************************************************************/

int btm_cmp(
	const struct btm *const restrict d1,
	const struct btm *const restrict d2
)
{
  int rc;
  
  assert(d1 != NULL);
  assert(d2 != NULL);
  
  if ((rc = d1->year  - d2->year))  return(rc);
  if ((rc = d1->month - d2->month)) return(rc);
  if ((rc = d1->day   - d2->day))   return(rc);
  if ((rc = d1->part  - d2->part))  return(rc);
  
  return(0);
}

/***********************************************************************/

int btm_cmp_date(
	const struct btm *const restrict d1,
	const struct btm *const restrict d2
)
{
  int rc;
  
  assert(d1 != NULL);
  assert(d2 != NULL);
  
  if ((rc = d1->year  - d2->year))  return(rc);
  if ((rc = d1->month - d2->month)) return(rc);
  if ((rc = d1->day   - d2->day))   return(rc);
  
  return(0);
}

/***************************************************************************/

void tm_init(struct tm *const ptm)
{
  memset(ptm,0,sizeof(struct tm));
  ptm->tm_isdst = -1;
}

/**************************************************************************/

