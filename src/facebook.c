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

#include <syslog.h>
#include <curl/curl.h>

#include "frontend.h"
#include "backend.h"
#include "globals.h"

/*********************************************************************/

static void	add_post_variable	(FILE *,const char *,const char *,bool);

/************************************************************************/

void notify_facebook(Request req)
{
  FILE    *out;
  CURL    *curl;
  char    *credentials;
  char    *token;
  char    *status      = NULL;
  char    *post        = NULL;
  char     url[BUFSIZ];
  size_t   size;
  CURLcode rc;
  
  assert(req != NULL);
  
  curl_global_init(CURL_GLOBAL_ALL);
  curl  = curl_easy_init();
  if (curl == NULL) return;

  credentials = NULL;
  size        = 0;
  out         = open_memstream(&credentials,&size);
  if (out == NULL) goto notify_facebook_error;
  
  add_post_variable(out,"grant_type"    ,"client_credentials",true);
  add_post_variable(out,"client_id"     ,c_facebook_ap_id    ,true);
  add_post_variable(out,"client_secret" ,c_facebook_ap_secret,false);
  fclose(out);
  
  token = NULL;
  size  = 0;
  out   = fopen("/dev/null","w");

  if (out == NULL) goto notify_facebook_error;
  curl_easy_setopt(curl,CURLOPT_VERBOSE,0L);
  curl_easy_setopt(curl,CURLOPT_URL,"https://graph.facebook.com/oauth/access_token");
  curl_easy_setopt(curl,CURLOPT_POSTFIELDS,credentials);
  curl_easy_setopt(curl,CURLOPT_NOPROGRESS,1L);
  curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,0);
  curl_easy_setopt(curl,CURLOPT_WRITEDATA,out);
  rc = curl_easy_perform(curl);
  fclose(out);

  curl_easy_cleanup(curl);
  
  if (rc != 0)
  {
    syslog(LOG_ERR,"curl_easy_perform(AUTH) = %s",curl_easy_strerror(rc));
    
    goto notify_facebook_error;
  }
  
  snprintf(url,sizeof(url),"https://graph.facebook.com/%s/feed",c_facebook_user);
  
  status = NULL;
  size   = 0;
  out    = open_memstream(&status,&size);
  if (out == NULL) goto notify_facebook_error;
  
  fprintf(
        out,
  	"%s: %s%s/%04d/%02d/%02d.%d",
  	req->status,
  	c_fullbaseurl,
  	c_baseurl,
  	g_blog->last.year,
  	g_blog->last.month,
  	g_blog->last.day,
  	g_blog->last.part
  );
  fclose(out);
  
  post   = NULL;
  size   = 0;
  out    = open_memstream(&post,&size);
  
  if (out == NULL) goto notify_facebook_error;
  fprintf(out,"%s&",token);
  add_post_variable(out,"message",status,false);
  fclose(out);

  out = fopen("/dev/null","w");
  if (out == NULL) goto notify_facebook_error;
  
  curl = curl_easy_init();
  if (curl == NULL) goto notify_facebook_error;
  
  curl_easy_setopt(curl,CURLOPT_VERBOSE,0L);
  curl_easy_setopt(curl,CURLOPT_URL,url);
  curl_easy_setopt(curl,CURLOPT_POSTFIELDS,post);
  curl_easy_setopt(curl,CURLOPT_NOPROGRESS,1L);
  curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,0);
  curl_easy_setopt(curl,CURLOPT_WRITEDATA,out);
  rc = curl_easy_perform(curl);
  if (rc != 0)
    syslog(LOG_ERR,"curl_easy_perform(POST) = %s",curl_easy_strerror(rc));

notify_facebook_error:
  free(post);
  free(status);
  free(token);
  free(credentials);
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
