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

#define HTTP_CONTINUE		100		/* 1.1	*/
#define HTTP_SWITCHPROTO	101		/* 1.1	*/
#define HTTP_100MAX		101

	/*--------------------------------------
	; Successful Codes
	;--------------------------------------*/
	
#define HTTP_OKAY		200		/* 1.0	*/
#define HTTP_CREATED		201		/* 1.0	*/
#define HTTP_ACCEPTED		202		/* 1.0	*/
#define HTTP_NONAUTH		203		/* 1.1	*/
#define HTTP_NOCONTENT		204		/* 1.0	*/
#define HTTP_RESETCONTENT	205		/* 1.1	*/
#define HTTP_PARTIALCONTENT	206		/* 1.1	*/
#define HTTP_200MAX		206

	/*----------------------------------------
	; Redirection Codes
	;----------------------------------------*/

#define HTTP_300		300
#define HTTP_MULTICHOICE	300		/* 1.1	*/
#define HTTP_MOVEPERM		301		/* 1.0	*/
#define HTTP_MOVETEMP		302		/* 1.0	*/
#define HTTP_SEEOTHER		303		/* 1.1	*/
#define HTTP_NOTMODIFIED	304		/* 1.0	*/
#define HTTP_USEPROXY		305		/* 1.1	*/
#define HTTP_300MAX		305

	/*-----------------------------------------
	; Client Error
	;----------------------------------------*/
	
#define HTTP_BADREQ		400		/* 1.0	*/
#define HTTP_UNAUTHORIZED	401		/* 1.0	*/
#define HTTP_PAYMENTREQ		402		/* 1.1	*/
#define HTTP_FORBIDDEN		403		/* 1.0	*/
#define HTTP_NOTFOUND		404		/* 1.0	*/
#define HTTP_METHODNOTALLOWED	405		/* 1.1	*/
#define HTTP_NOTACCEPTABLE	406		/* 1.1	*/
#define HTTP_PROXYAUTHREQ	407		/* 1.1	*/
#define HTTP_TIMEOUT		408		/* 1.1	*/
#define HTTP_CONFLICT		409		/* 1.1	*/
#define HTTP_GONE		410		/* 1.1	*/
#define HTTP_LENGTHREQ		411		/* 1.1	*/
#define HTTP_PRECONDITION	412		/* 1.1	*/
#define HTTP_TOOLARGE		413		/* 1.1	*/
#define HTTP_URITOOLONG		414		/* 1.1	*/
#define HTTP_MEDIATYPE		415		/* 1.1	*/
#define HTTP_RANGEERROR		416		/* 1.1	*/
#define HTTP_EXPECTATION	417		/* 1.1	*/
#define HTTP_400MAX		417

	/*-----------------------------------------
	; Server Errors
	;-----------------------------------------*/
	
#define HTTP_ISERVERERR		500		/* 1.0	*/
#define HTTP_NOTIMP		501		/* 1.0	*/
#define HTTP_BADGATEWAY		502		/* 1.0	*/
#define HTTP_NOSERVICE		503		/* 1.0	*/
#define HTTP_GATEWAYTIMEOUT	504		/* 1.1	*/
#define HTTP_HTTPVERSION	505		/* 1.1	*/
#define HTTP_500MAX		505

extern const struct chunk_callback m_callbacks[];
extern const size_t                m_cbnum;

char	*get_remote_user		(void);
bool	 authenticate_author		(Request);
int	 generate_pages			(Request);
void	 notify_facebook		(Request);
void	 notify_emaillist		(void);
int	 tumbler_page			(FILE *,Tumbler);
int	 BlogDatesInit			(void);
int	 entry_add			(Request);
void	 fix_entry			(Request);
void	 generic_cb			(const char *,FILE *,void *);
void	 dbcritical			(char *);
char	*entity_conversion		(const char *);
char	*entity_encode			(const char *);
FILE	*fentity_encode_onread		(FILE *);
FILE	*fentity_encode_onwrite		(FILE *);
size_t	 fcopy				(FILE *,FILE *);

#endif

