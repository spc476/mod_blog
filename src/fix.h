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

#ifndef FIX_H
#define FIX_H

#include <cgilib/errors.h>
#include "chunk.h"

#define DB_BLOCK	1024

extern struct chunk_callback m_callbacks[];
extern size_t                m_cbnum;

int	authenticate_author		(Request);
int	generate_pages			(Request);
void	notify_weblogcom		(void);
void	notify_emaillist		(void);
int	primary_page			(Stream,int,int,int);
int	tumbler_page			(Stream,Tumbler);
int	BlogDatesInit			(void);
int	entry_add			(Request);
void	fix_entry			(Request);
void	generic_cb			(const char *,Stream,void *);
void	dbcritical			(char *);

#endif

