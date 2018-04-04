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

#ifndef I_923B1CED_AA68_5BFE_A50A_C27EEE5A2EE8
#define I_923B1CED_AA68_5BFE_A50A_C27EEE5A2EE8

#include <cgilib6/chunk.h>

#include "frontend.h"

#define DB_BLOCK        1024

extern char *get_remote_user        (void);
extern bool  authenticate_author    (Request *);
extern void  notify_emaillist       (Request *);
extern int   entry_add              (Request *);
extern void  fix_entry              (Request *);
extern void  generic_cb             (char const *,FILE *,void *);
extern void  dbcritical             (char const *);
extern char *entity_conversion      (char const *);
extern char *entity_encode          (char const *);
extern FILE *fentity_encode_onread  (FILE *);
extern FILE *fentity_encode_onwrite (FILE *);

#endif
