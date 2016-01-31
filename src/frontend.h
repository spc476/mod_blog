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

#ifndef I_CA412E0F_D512_5F3E_AB0B_4739574108FF
#define I_CA412E0F_D512_5F3E_AB0B_4739574108FF

#include <cgilib6/cgi.h>

#include "wbtum.h"
#include "timeutil.h"
#include "blog.h"

typedef struct rflags
{
  unsigned int emailin    : 1;
  unsigned int update     : 1;
  unsigned int preview    : 1;
  unsigned int regenerate : 1;
} RFlags;

typedef struct request
{
  int       (*command)(struct request *);
  int       (*error)  (struct request *,int,const char *, ... );
  RFlags      f;
  Cgi         cgi;
  FILE       *in;
  FILE       *out;
  char       *update;
  char       *origauthor;
  char       *author;
  char       *name;
  char       *title;
  char       *class;
  char       *status;
  char       *date;
  char       *origbody;
  char       *body;
  char       *reqtumbler;
  tumbler__s  tumbler;
} *Request;  

typedef struct dflags
{
  unsigned int fullurl    : 1;
  unsigned int reverse    : 1;
  unsigned int navigation : 1;
  unsigned int navprev    : 1;
  unsigned int navnext    : 1;
  unsigned int edit       : 1;
  unsigned int overview   : 1;
} DFlags;

typedef struct display
{
  DFlags      f;
  unit__e     navunit;
  FILE       *htmldump;
  Cgi         cgi;
  Request     req;
  struct btm  previous;
  struct btm  next;
} *Display;

struct callback_data
{
  List       list;
  struct btm last;	/* timestamp of previous entry */
  BlogEntry  entry;	/* current entry being processed */
  FILE      *ad;	/* file containing ad */
  char      *adtag;
};

#endif

