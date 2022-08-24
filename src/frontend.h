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
#include "config.h"

typedef struct request
{
  char        *origauthor;
  char        *author;
  char        *title;
  char        *class;
  char        *status;
  char        *date;
  char        *adtag;
  char        *origbody;
  char        *body;
  char        *reqtumbler;
  struct btm   when;
  tumbler__s   tumbler;
  void       (*conversion)(FILE *restrict,FILE *restrict);
  struct
  {
    unsigned int fullurl    : 1;
    unsigned int reverse    : 1;
    unsigned int navigation : 1;
    unsigned int navprev    : 1;
    unsigned int navnext    : 1;
    unsigned int edit       : 1;
    unsigned int cgi        : 1;
    unsigned int htmldump   : 1;
    unsigned int email      : 1;
  } f;  
} Request;

/************************************************/

extern char *get_remote_user        (void);
extern bool  authenticate_author    (Request *,config__s const *);
extern void  notify_emaillist       (Request *,config__s const *);
extern bool  entry_add              (Request *,config__s const *,Blog *);
extern void  fix_entry              (Request *);
extern char *entity_conversion      (char const *);
extern char *entity_encode          (char const *);
extern FILE *fentity_encode_onread  (FILE *);
extern FILE *fentity_encode_onwrite (FILE *);

#endif
