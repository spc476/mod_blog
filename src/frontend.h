/************************************************************************
*
* Copyright 2005 by Sean Conner.  All Rights Reserved.
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
*************************************************************************/

#ifndef FRONTEND_H
#define FRONTEND_H

#include <time.h>
#include <cgilib/cgi.h>
#include "wbtum.h"
#include "timeutil.h"
#include "blog.h"

typedef struct rflags
{
  unsigned int std_in     : 1;
  unsigned int emailin    : 1;
  unsigned int filein     : 1;
  unsigned int cgiin      : 1;
  unsigned int update     : 1;
  unsigned int preview    : 1;
  unsigned int regenerate : 1;
} RFlags;

typedef struct request
{
  int     (*command)(struct request *);
  int     (*error)  (struct request *,int,char *,char *, ... );
  RFlags    f;
  Cgi       cgi;
  Stream    in;
  Stream    out;
  char     *update;
  char     *origauthor;
  char     *author;
  char     *name;
  char     *title;
  char     *class;
  char     *date;
  char     *origbody;
  char     *body;
  char     *reqtumbler;
  Tumbler  tumbler;
} *Request;  

typedef struct dflags
{
  unsigned int fullurl    : 1;
  unsigned int reverse    : 1;
  unsigned int cgi        : 1;
  unsigned int navigation : 1;
  unsigned int navprev    : 1;
  unsigned int navnext    : 1;
  unsigned int edit       : 1;
} DFlags;

typedef struct display
{
  DFlags      f;
  int         navunit;
  char       *adtag;
  Stream      htmldump;
  Cgi         cgi;
  Request     req;
  struct btm  begin;
  struct btm  now;
  struct btm  updatetime;
  struct btm  previous;
  struct btm  next;
  time_t      tst;	/* Time_t based Start Time */
  struct tm   stmst;	/* Struct TM based Start Time */
#if 0
  struct tm              begin;
  struct tm              now;
  struct tm              updatetime;
  struct tm              previous;
  struct tm              next;
#endif
} *Display;

struct callback_data
{
  List       list;
  struct btm last;	/* timestamp of previous entry */
  BlogEntry  entry;	/* current entry being processed */
};

#endif

