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
*********************************************************************/

#ifndef I_18C80596_12AD_588C_8AC5_C30F6FDDBF24
#define I_18C80596_12AD_588C_8AC5_C30F6FDDBF24

#include <stdbool.h>
#include <stdio.h>
#include "frontend.h"

typedef void (*conversion__f)(FILE *restrict,FILE *restrict);

extern bool           TO_email             (char const *,bool);
extern conversion__f  TO_conversion        (char const *,char const *);
extern void           no_conversion        (FILE *restrict,FILE *restrict);
extern void           text_conversion      (FILE *restrict,FILE *restrict);
extern void           mixed_conversion     (FILE *restrict,FILE *restrict);
extern void           html_conversion      (FILE *restrict,FILE *restrict);
extern void           buff_conversion      (FILE *restrict,FILE *restrict);
extern FILE          *fjson_encode_onwrite (FILE *);

#endif
