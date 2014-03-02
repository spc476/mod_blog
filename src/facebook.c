/*********************************************************************
*
* Copyright 2010 by Sean Conner.  All Rights Reserved.
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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <curl/curl.h>

#include "frontend.h"
#include "backend.h"
#include "globals.h"

/*********************************************************************/

static void	add_post_variable	(FILE *,const char *,const char *,bool);

/************************************************************************/

void notify_facebook(Request req)
{
  FILE   *out;
  CURL   *curl;
  char   *credentials;
  char   *token;
  char   *status;
  char   *post;
  char    url[BUFSIZ];
  size_t  size;
  
  assert(req != NULL);
  
  curl_global_init(CURL_GLOBAL_ALL);
  
  credentials = NULL;
  size        = 0;
  out         = open_memstream(&credentials,&size);
  
  if (out == NULL) return;
  add_post_variable(out,"grant_type"    ,"client_credentials",true);
  add_post_variable(out,"client_id"     ,c_facebook_ap_id    ,true);
  add_post_variable(out,"client_secret" ,c_facebook_ap_secret,false);
  
  fclose(out);

  curl  = curl_easy_init();
  if (curl == NULL) return;
  
  token = NULL;
  size  = 0;
  out   = open_memstream(&token,&size);

  if (out == NULL) return;
  curl_easy_setopt(curl,CURLOPT_VERBOSE,0L);
  curl_easy_setopt(curl,CURLOPT_URL,"https://graph.facebook.com/oauth/access_token");
  curl_easy_setopt(curl,CURLOPT_POSTFIELDS,credentials);
  curl_easy_setopt(curl,CURLOPT_NOPROGRESS,1L);
  curl_easy_setopt(curl,CURLOPT_WRITEDATA,out);
  curl_easy_perform(curl);

  fclose(out);

  snprintf(url,sizeof(url),"https://graph.facebook.com/%s/feed",c_facebook_user);
  
  status = NULL;
  size   = 0;
  out    = open_memstream(&status,&size);
  if (out == NULL) return;
  
  fprintf(
        out,
  	"%s: %s%s/%04d/%02d/%02d.%d",
  	req->status,
  	c_fullbaseurl,
  	c_baseurl,
  	g_blog->now.year,
  	g_blog->now.month,
  	g_blog->now.day,
  	g_blog->now.part
  );
  fclose(out);
  
  post   = NULL;
  size   = 0;
  out    = open_memstream(&post,&size);
  
  if (out == NULL) return;
  fprintf(out,"%s&",token);
  add_post_variable(out,"message",status,false);
  fclose(out);

  curl_easy_setopt(curl,CURLOPT_URL,url);
  curl_easy_setopt(curl,CURLOPT_POSTFIELDS,post);
  curl_easy_setopt(curl,CURLOPT_WRITEDATA,out);
  curl_easy_perform(curl);

  free(post);
  free(status);
  free(token);
  free(credentials);
  curl_easy_cleanup(curl);
}

/*************************************************************************/

static void add_post_variable(FILE *out,const char *name,const char *value,bool more)
{
  char *encode_name;
  char *encode_value;
  char *encode_sep;
  
  assert(out   != NULL);
  assert(name  != NULL);
  assert(value != NULL);
  
  encode_name  = UrlEncodeString(name);
  encode_value = UrlEncodeString(value);
  encode_sep   = more ? "&" : "";
  
  fprintf(out,"%s=%s%s",encode_name,encode_value,encode_sep);
  
  free(encode_value);
  free(encode_name);
}

/***********************************************************************/
