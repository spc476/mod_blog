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

#include <cgilib6/chunk.h>

#include "frontend.h"

#define DB_BLOCK	1024

	/*-----------------------------------
	; Informational Codes
	;----------------------------------*/

typedef enum http__e
{
  HTTP_CONTINUE		= 100,		/* 1.1	*/
  HTTP_SWITCHPROTO	= 101,		/* 1.1	*/
  HTTP_100MAX		= 101,

	/*--------------------------------------
	; Successful Codes
	;--------------------------------------*/
	
  HTTP_OKAY		= 200,		/* 1.0	*/
  HTTP_CREATED		= 201,		/* 1.0	*/
  HTTP_ACCEPTED		= 202,		/* 1.0	*/
  HTTP_NONAUTH		= 203,		/* 1.1	*/
  HTTP_NOCONTENT	= 204,		/* 1.0	*/
  HTTP_RESETCONTENT	= 205,		/* 1.1	*/
  HTTP_PARTIALCONTENT	= 206,		/* 1.1	*/
  HTTP_200MAX		= 206,

	/*----------------------------------------
	; Redirection Codes
	;----------------------------------------*/

  HTTP_300		= 300,
  HTTP_MULTICHOICE	= 300,		/* 1.1	*/
  HTTP_MOVEPERM		= 301,		/* 1.0	*/
  HTTP_MOVETEMP		= 302,		/* 1.0	*/
  HTTP_SEEOTHER		= 303,		/* 1.1	*/
  HTTP_NOTMODIFIED	= 304,		/* 1.0	*/
  HTTP_USEPROXY		= 305,		/* 1.1	*/
  HTTP_300MAX		= 305,

	/*-----------------------------------------
	; Client Error
	;----------------------------------------*/
	
  HTTP_BADREQ		= 400,		/* 1.0	*/
  HTTP_UNAUTHORIZED	= 401,		/* 1.0	*/
  HTTP_PAYMENTREQ	= 402,		/* 1.1	*/
  HTTP_FORBIDDEN	= 403,		/* 1.0	*/
  HTTP_NOTFOUND		= 404,		/* 1.0	*/
  HTTP_METHODNOTALLOWED	= 405,		/* 1.1	*/
  HTTP_NOTACCEPTABLE	= 406,		/* 1.1	*/
  HTTP_PROXYAUTHREQ	= 407,		/* 1.1	*/
  HTTP_TIMEOUT		= 408,		/* 1.1	*/
  HTTP_CONFLICT		= 409,		/* 1.1	*/
  HTTP_GONE		= 410,		/* 1.1	*/
  HTTP_LENGTHREQ	= 411,		/* 1.1	*/
  HTTP_PRECONDITION	= 412,		/* 1.1	*/
  HTTP_TOOLARGE		= 413,		/* 1.1	*/
  HTTP_URITOOLONG	= 414,		/* 1.1	*/
  HTTP_MEDIATYPE	= 415,		/* 1.1	*/
  HTTP_RANGEERROR	= 416,		/* 1.1	*/
  HTTP_EXPECTATION	= 417,		/* 1.1	*/
  HTTP_400MAX		= 417,

	/*-----------------------------------------
	; Server Errors
	;-----------------------------------------*/
	
  HTTP_ISERVERERR	= 500,		/* 1.0	*/
  HTTP_NOTIMP		= 501,		/* 1.0	*/
  HTTP_BADGATEWAY	= 502,		/* 1.0	*/
  HTTP_NOSERVICE	= 503,		/* 1.0	*/
  HTTP_GATEWAYTIMEOUT	= 504,		/* 1.1	*/
  HTTP_HTTPVERSION 	= 505,		/* 1.1	*/
  HTTP_500MAX		= 505,
} http__e;

extern char	*get_remote_user		(void);
extern bool	 authenticate_author		(Request);
extern int	 generate_pages			(Request);
extern void	 notify_emaillist		(void);
extern int	 tumbler_page			(FILE *,Tumbler);
extern int	 entry_add			(Request);
extern void	 fix_entry			(Request);
extern void	 generic_cb			(const char *,FILE *,void *);
extern void	 dbcritical			(char *);
extern char	*entity_conversion		(const char *);
extern char	*entity_encode			(const char *);
extern FILE	*fentity_encode_onread		(FILE *);
extern FILE	*fentity_encode_onwrite		(FILE *);
extern size_t	 fcopy				(FILE *,FILE *);

#endif

