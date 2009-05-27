/************
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
***************************/

#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <cgilib/ddt.h>

#include "timeutil.h"

/*************************************************************************/

const char *const nth(unsigned long n)
{
  switch((int)(n % 20UL))
  {
    case 1:
         return("st");
    case 2:
         return("nd");
    case 3:
         return("rd");
    default:
         return("th");
  }
}

/**********************************************************************/

int max_monthday(int year,int month)
{
  static const int days[] = { 31,0,31,30,31,30,31,31,30,31,30,31 } ;
  
  ddt(year  >  0);
  ddt(month >  0);
  ddt(month < 13);

  if (month == 2)
  {
    /*---------------------------------------------------
    ; in case you didn't know, leap years are those years
    ; that are divisible by 4, except if it's divisible by 
    ; 100, then it's not, unless it's divisible by 400, then
    ; it is.  1800 and 1900 were NOT leap years, but 2000 is.
    ;------------------------------------------------------*/
    
    if ((year % 400) == 0) return(29);
    if ((year % 100) == 0) return(28);
    if ((year %   4) == 0) return(29);
    return(28);
  }
  else
    return(days[month - 1]);
}

/***************************************************************************/

void month_add(struct tm *ptm)
{
  ddt(ptm != NULL);
  
  ptm->tm_mon++;
  if (ptm->tm_mon > 11)
  {
    ptm->tm_mon = 0;
    ptm->tm_year++;
  }
}

/**************************************************************************/
  
void day_add(struct tm *ptm)
{
  ddt(ptm != NULL);

  ptm->tm_mday++;
  if (ptm->tm_mday > max_monthday(ptm->tm_year + 1900,ptm->tm_mon + 1))
  {
    ptm->tm_mday = 1;
    month_add(ptm);
  }
}

/**************************************************************************/

void month_sub(struct tm *ptm)
{
  ddt(ptm != NULL);
  
  ptm->tm_mon--;
  if (ptm->tm_mon < 0)
  {
    ptm->tm_year--;
    ptm->tm_mon = 11;
  }
}

/*************************************************************************/

void day_sub(struct tm *ptm)
{
  ddt(ptm != NULL);

  ptm->tm_mday--;
  if (ptm->tm_mday == 0)
  {
    month_sub(ptm);
    ptm->tm_mday = max_monthday(ptm->tm_year + 1900,ptm->tm_mon + 1);
  }
}

/*************************************************************************/

void tm_init(struct tm *ptm)
{
  memset(ptm,0,sizeof(struct tm));
  ptm->tm_isdst = -1;
}

/**************************************************************************/

int tm_cmp(struct tm *ptm1,struct tm *ptm2)
{
  int rc;

  ddt(ptm1 != NULL);
  ddt(ptm2 != NULL);

  if ((rc = ptm1->tm_year - ptm2->tm_year)) return(rc);
  if ((rc = ptm1->tm_mon  - ptm2->tm_mon))  return(rc);
  if ((rc = ptm1->tm_mday - ptm2->tm_mday)) return(rc);

  return(0);
}

/**************************************************************************/

void tm_to_tm(struct tm *ptm)
{
  ddt(ptm != NULL);
  
  /*--------------------------------------------------------------
  ; Assume the struct tm is normalized for blog use, and correct 
  ; for use in ANSI C calls.
  ;---------------------------------------------------------------*/
  
  ptm->tm_year -= 1900;
  ptm->tm_mon--;
  mktime(ptm);

  /*-------------------------------------------------------
  ; think I finally tracked down a bug here.  mktime() takes
  ; into account Daylight Saving Time, so if it is in effect,
  ; we need to back-adjust for it.  Sigh.
  ;-------------------------------------------------------*/

  if (ptm->tm_isdst) ptm->tm_hour--;
}

/**************************************************************************/

void tm_to_blog(struct tm *ptm)
{
  /*----------------------------------------------------------------
  ; Assume the struct tm is normalized for ANSI C use, correct for
  ; use in the blog.
  ;----------------------------------------------------------------*/
  
  mktime(ptm);
  ptm->tm_year += 1900;
  ptm->tm_mon ++;  
}

/**************************************************************************/

void tm_fromstring(struct tm *ptm,char *s)
{
  ddt(ptm != NULL);
  ddt(s   != NULL);
  
  tm_init(ptm);
  ptm->tm_year = strtol(s,&s,10) - 1900;
  if (*s++ == '\0') return;
  ptm->tm_mon  = strtol(s,&s,10) - 1;
  if (*s++ == '\0') return;
  ptm->tm_mday = strtol(s,&s,10);
  if (*s == '\0') return;
  if (*s++ == '/') return;
  ptm->tm_hour = strtol(s,&s,10);
}

/*************************************************************************/

long days_between(struct tm *start,struct tm *end)
{
  double seconds;
  long   days;
  
  ddt(start != NULL);
  ddt(end   != NULL);
  
  seconds = difftime(mktime(end),mktime(start));
  days    = (long)(seconds / (60.0 * 60.0 * 24.0));
  return(days);
}

/************************************************************************/

