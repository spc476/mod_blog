/*********************************************************************
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
**********************************************************************/

#ifndef WBTUM_H
#define WBTUM_H

#include <stdint.h>
#include <cgilib6/nodelist.h>

/**********************************************************************/

enum ttypes
{
  TUMBLER_SINGLE,
  TUMBLER_RANGE
};

enum
{
  YEAR,
  MONTH,
  DAY,
  PART,
  INDEX
};

typedef struct tt
{
  unsigned int file     : 1;
  unsigned int redirect : 1;
  unsigned int error    : 1;
} TFlags;

typedef struct tumbler
{
  size_t pairs;
  TFlags flags;
  List   units;
} *Tumbler;

typedef struct tumunit
{
  Node         node;
  size_t       size;
  enum ttypes  type;
  int          entry[4];
  char        *file;  
} *TumblerUnit;

/***********************************************************************/

int		 (TumblerNew)		(Tumbler *,char **);
char		*(TumblerCanonical)	(Tumbler);
int		 (TumblerFree)		(Tumbler *);

#endif

