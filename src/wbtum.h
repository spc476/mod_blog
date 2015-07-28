/*********************************************************************
*
* Copyright 2015 by Sean Conner.  All Rights Reserved.
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
**********************************************************************/

#ifndef WBTUM_H
#define WBTUM_H

#include <stdbool.h>
#include <stdio.h>
#include "timeutil.h"

/**********************************************************************/

#define ENTRY_MAX	23

typedef enum unit__e
{
  UNIT_YEAR,
  UNIT_MONTH,
  UNIT_DAY,
  UNIT_PART,
  UNIT_INDEX
} unit__e;

typedef struct tumbler__s
{
  struct btm start;	/* starting date			*/
  struct btm stop;	/* ending date				*/
  unit__e    ustart;	/* ending segment of initial part	*/
  unit__e    ustop;	/* ending segment of range part		*/
  int        segments;	/* used for range part			*/
  bool       file;	/* a file was specified			*/
  bool       redirect;	/* we need to redirect			*/
  bool       range;	/* a range was specified		*/
  char       filename[FILENAME_MAX];
} tumbler__s;

/***********************************************************************/

extern bool  tumbler_new	(tumbler__s *const,const char *);
extern char *tumbler_canonical	(const tumbler__s *const);

#endif
