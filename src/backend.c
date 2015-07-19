/***********************************************
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
************************************************/

#ifndef __GNUC__
#  define __attribute__(x)
#endif

#define _GNU_SOURCE 1

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>

#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>

#include <cgilib6/htmltok.h>
#include <cgilib6/util.h>
#include <cgilib6/cgi.h>
#include <cgilib6/chunk.h>

#include "conf.h"
#include "blog.h"
#include "timeutil.h"
#include "wbtum.h"
#include "frontend.h"
#include "fix.h"
#include "blogutil.h"
#include "backend.h"
#include "globals.h"

/*****************************************************************/

static void	   calculate_previous		(const struct btm);
static void	   calculate_next		(const struct btm);
static const char *mime_type			(const char *);
static int	   display_file			(FILE *,tumbler__s *);
static char       *tag_collect			(List *);
static char	  *tag_pick                     (const char *);
static void	   free_entries			(List *);

/************************************************************************/

int generate_pages(Request req __attribute__((unused)))
{
  for (size_t i = 0 ; i < c_numtemplates; i++)
  {
    FILE *out;
    
    out = fopen(c_templates[i].file,"w");
    if (out == NULL)
    {
      syslog(LOG_ERR,"%s: %s",c_templates[i].file,strerror(errno));
      continue;
    }
    
    (*c_templates[i].pagegen)(&c_templates[i],out,g_blog);
    fclose(out);
  }
  
  return 0;
}

/******************************************************************/

int pagegen_items(
	const template__t *const restrict template,
	FILE              *const restrict out,
	Blog               const restrict blog
)
{
  struct btm            thisday;
  char                 *tags;
  struct callback_data  cbd;
  
  assert(template != NULL);
  assert(out      != NULL);
  assert(blog     != NULL);
  
  g_templates  = template->template;
  gd.f.fullurl = template->fullurl;
  gd.f.reverse = template->reverse;
  thisday      = blog->now;
  
  memset(&cbd,0,sizeof(struct callback_data));
  
  ListInit(&cbd.list);
  if (template->reverse)
    BlogEntryReadXD(g_blog,&cbd.list,&thisday,template->items);
  else
    BlogEntryReadXU(g_blog,&cbd.list,&thisday,template->items);
  
  tags      = tag_collect(&cbd.list);
  cbd.adtag = tag_pick(tags);
  free(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);
  free(cbd.adtag);
  return 0;
}

/************************************************************************/

int pagegen_days(
	const template__t *const restrict template,
	FILE              *const restrict out,
	Blog               const restrict blog
)
{
  struct btm            thisday;
  size_t                days;
  char                 *tags;
  struct callback_data  cbd;
  bool                  added;
  
  assert(template != NULL);
  assert(out      != NULL);
  assert(blog     != NULL);

  g_templates  = template->template;
  gd.f.fullurl = false;
  gd.f.reverse = true;
  thisday      = blog->now;
  
  memset(&cbd,0,sizeof(struct callback_data));
  ListInit(&cbd.list);
  
  for (days = 0 , added = false ; days < template->items ; )
  {
    BlogEntry entry;
    
    if (btm_cmp(&thisday,&g_blog->first) < 0) break;
    
    entry = BlogEntryRead(g_blog,&thisday);
    if (entry)
    {
      assert(entry->valid);
      ListAddTail(&cbd.list,&entry->node);
      added = true;
    }
    
    thisday.part--;
    if (thisday.part == 0)
    {
      thisday.part = ENTRY_MAX;
      btm_dec_day(&thisday);
      if (added)
        days++;
      added = false;
    }
  }
  
  tags      = tag_collect(&cbd.list);
  cbd.adtag = tag_pick(tags);
  free(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);
  free(cbd.adtag);
  return 0;
}

/************************************************************************/

int tumbler_page(FILE *out,tumbler__s *spec)
{
  struct callback_data cbd;
  struct btm           start;
  struct btm           end;
  char                *tags;
  
  assert(out  != NULL);
  assert(spec != NULL);
  
  gd.f.fullurl = false; /* XXX why? */
  
  if (spec->redirect)
    return HTTP_NOTIMP;
  
  if (spec->file)
  {
    display_file(out,spec);
    return 0; /* XXX hack for now */
  }
  
  memset(&cbd,0,sizeof(cbd));
  ListInit(&cbd.list);
  
  /*----------------------------------------------------------------------
  ; from here to the comment about sanity checking replaced around 100 very
  ; confused lines of code.  The tumbler code now does more checking of data
  ; than the old version, so a lot of code was removed.
  ;-----------------------------------------------------------------------*/
  
  assert(spec->start.year  >= g_blog->first.year);
  assert(spec->start.month >   0);
  assert(spec->start.month <  13);
  assert(spec->start.day   >   0);
  assert(spec->start.day   <= max_monthday(spec->start.year,spec->start.month));

  assert(spec->stop.year  >= g_blog->first.year);
  assert(spec->stop.month >   0);
  assert(spec->stop.month <  13);
  assert(spec->stop.day   >   0);
  assert(spec->stop.day   <= max_monthday(spec->stop.year,spec->stop.month));

  start = spec->start;
  end   = spec->stop;
  
  if (!spec->range)
  {
    gd.f.navigation = true;
    gd.navunit      = spec->ustart > spec->ustop
                    ? spec->ustart
                    : spec->ustop
                    ;
    calculate_previous(start);
    calculate_next(end);
  }
  else
  {
    if (btm_cmp(&spec->start,&spec->stop) > 0)
    {
      struct btm tmp;
      
      gd.f.reverse = true;
      tmp          = start;
      start        = end;
      end          = start;
      
      if ((start.part == ENTRY_MAX) && (end.part == 1))
      {
        start.part = 1;
        end.part   = ENTRY_MAX;
      }
    }
  }

  if (end.day > max_monthday(end.year,end.month))
    return(1);

  /*---------------------------------------------------------------------
  ; Okay, sanity checking here ... These should be true once we hit this
  ; spot of the code.
  ;---------------------------------------------------------------------*/
  
  assert(start.year  >= 1);
  assert(start.month >= 1);
  assert(start.month <= 12);
  assert(start.day   >= 1);
  assert(start.day   <= max_monthday(start.year,start.month));
  
  assert(end.year  >= 1);
  assert(end.month >= 1);
  assert(end.month <= 12);
  assert(end.day   >= 1);
  assert(end.day   <= max_monthday(end.year,end.month));
  
  assert(start.year  <= end.year);
  assert(start.month <= end.month);
  assert(start.day   <= end.day);
  assert(start.part  <= end.part);
  
  /*-------------------------------------------------------------------------
  ; okay, resume processing ... bound against the starting time of the blog,
  ; and the current time.
  ;    
  ; From here on out, it's pretty straight forward.  read a day, if it has
  ; entries, add it to the list, otherwise, continue on, advancing (or
  ; retreading) the days as we go ...
  ;
  ; these four lines replaced 65 very confused lines of code.
  ;-----------------------------------------------------------*/
  
  if (gd.f.reverse)
    BlogEntryReadBetweenD(g_blog,&cbd.list,&end,&start);
  else
    BlogEntryReadBetweenU(g_blog,&cbd.list,&start,&end);
  
  tags      = tag_collect(&cbd.list);  
  cbd.adtag = tag_pick(tags);
  free(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);
  free(cbd.adtag);
  return(0);  
}

/******************************************************************/

static void calculate_previous(const struct btm start)
{
  gd.previous = start;

  switch(gd.navunit)
  {
    case UNIT_YEAR:
         if (start.year == g_blog->first.year)
           gd.f.navprev = false;
         else
           gd.previous.year = start.year - 1;
         break;

    case UNIT_MONTH:
         if (
              (start.year == g_blog->first.year) 
              && (start.month == g_blog->first.month)
            )
           gd.f.navprev = false;
         else
	   btm_dec_month(&gd.previous);
         break;

    case UNIT_DAY:
         if (btm_cmp_date(&start,&g_blog->first) == 0)
           gd.f.navprev = false;
         else
         {
           btm_dec_day(&gd.previous);
           
           while(btm_cmp(&gd.previous,&g_blog->first) > 0)
           {
             BlogEntry entry;
             
             entry = BlogEntryRead(g_blog,&gd.previous);
             if (entry == NULL)
             {
               btm_dec_day(&gd.previous);
               continue;
             }
             assert(entry->valid);
             BlogEntryFree(entry);
             return;
           }
                      
           gd.f.navprev = false;
         }
         break;

    case UNIT_PART:
         if (btm_cmp(&start,&g_blog->first) == 0)
           gd.f.navprev = false;
         else
         {
	   btm_dec_part(&gd.previous);

           while(btm_cmp(&gd.previous,&g_blog->first) > 0)
           {
             BlogEntry entry;
             
             entry = BlogEntryRead(g_blog,&gd.previous);
             if (entry == NULL)
             {
	       btm_dec_part(&gd.previous);
               continue;
             }
             assert(entry->valid);
             BlogEntryFree(entry);
             return;
           }
           
           gd.f.navprev = false;
         }
         break;

    default:
         assert(0);
  }
}

/******************************************************************/

static void calculate_next(const struct btm end)
{
  gd.next = end;
  
  switch(gd.navunit)
  {
    case UNIT_YEAR:
         if (end.year == g_blog->now.year)
           gd.f.navnext = false;
         else
           gd.next.year  = end.year + 1;
         break;

    case UNIT_MONTH:
         if (
              (end.year == g_blog->now.year) 
              && (end.month == g_blog->now.month)
            )
           gd.f.navnext = false;
         else
           btm_inc_month(&gd.next);
         break;

    case UNIT_DAY:
         if (btm_cmp_date(&end,&g_blog->now) == 0)
           gd.f.navnext = false;
         else
         {
           btm_inc_day(&gd.next);
	   gd.next.part = 1;
           
           while(btm_cmp(&gd.next,&g_blog->now) <= 0)
           {
             BlogEntry entry;
             
             entry = BlogEntryRead(g_blog,&gd.next);
             if (entry == NULL)
             {
               btm_inc_day(&gd.next);
               continue;
             }
             assert(entry->valid);
             BlogEntryFree(entry);
             return;
           }
           
           gd.f.navnext = false;
         }
         break;

    case UNIT_PART:
         if (btm_cmp(&end,&g_blog->now) == 0)
           gd.f.navnext = false;
	 else
	 {
	   gd.next.part++;
         
           while(btm_cmp(&gd.next,&g_blog->now) <= 0)
           {
             BlogEntry entry;
             
             entry = BlogEntryRead(g_blog,&gd.next);
             if (entry == NULL)
             {
               gd.next.part = 1;
               btm_inc_day(&gd.next);
               continue;
             }
             assert(entry->valid);
             BlogEntryFree(entry);
             return;
           }
           gd.f.navnext = false;
	 }
         break;

    default:
         assert(0);
  }
}

/******************************************************************/

static int dstring_cmp(const void *needle,const void *haystack)
{
  const char           *key   = needle;
  const struct dstring *value = haystack;
  
  return strcmp(key,value->s1);
}

static const char *mime_type(const char *filename)
{
  static const struct dstring  types[] = 
  {
    { ".ai"		, "application/postscript"		} ,
    { ".aif"		, "audio/x-aiff"			} ,
    { ".aifc"		, "audio/x-aiff"			} ,
    { ".aiff"		, "audio/x-aiff"			} ,
    { ".asc"		, "text/plain"				} ,
    { ".au"		, "audio/basic"				} ,
    { ".avi"		, "video/x-msvideo"			} ,
    { ".bcpio"		, "application/x-bcpio"			} ,
    { ".bin"		, "application/octet-stream"		} ,
    { ".bmp"		, "image/bmp"				} ,
    { ".cdf"		, "application/x-netcdf"		} ,
    { ".cgm"		, "image/cgm"				} ,
    { ".class"		, "application/octet-stream"		} ,
    { ".cpio"		, "application/x-cpio"			} ,
    { ".cpt"		, "application/mac-compactpro"		} ,
    { ".csh"		, "application/x-csh"			} ,
    { ".css"		, "text/css"				} ,
    { ".dcr"		, "application/x-director"		} ,
    { ".dir"		, "application/x-director"		} ,
    { ".djv"		, "image/vnd.djvu"			} ,
    { ".djvu"		, "image/vnd.djvu"			} ,
    { ".dll"		, "application/octet-stream"		} ,
    { ".dms"		, "application/octet-stream"		} ,
    { ".doc"		, "application/msword"			} ,
    { ".dtd"		, "application/xml-dtd"			} ,
    { ".dvi"		, "application/x-dvi"			} ,
    { ".dxr"		, "application/x-director"		} ,
    { ".eps"		, "application/postscript"		} ,
    { ".etx"		, "text/x-setext"			} ,
    { ".exe"		, "application/octet-stream"		} ,
    { ".ez"		, "application/andrew-inset"		} ,
    { ".gif"		, "image/gif"				} ,
    { ".gram"		, "application/srgs"			} ,
    { ".grxml"		, "application/srgs+xml"		} ,
    { ".gtar"		, "application/x-gtar"			} ,
    { ".gz"		, "application/x-gzip"			} ,
    { ".hdf"		, "application/x-hdf"			} ,
    { ".hqx"		, "application/mac-binhex40"		} ,
    { ".htm"		, "text/html"				} ,
    { ".html"		, "text/html"				} ,
    { ".ice"		, "x-conference/x-cooltalk"		} ,
    { ".ico"		, "image/x-icon"			} ,
    { ".ics"		, "text/calendar"			} ,
    { ".ief"		, "image/ief"				} ,
    { ".ifb"		, "text/calendar"			} ,
    { ".iges"		, "model/iges"				} ,
    { ".igs"		, "model/iges"				} ,
    { ".jpe"		, "image/jpeg"				} ,
    { ".jpeg"		, "image/jpeg"				} ,
    { ".jpg"		, "image/jpeg"				} ,
    { ".js"		, "application/x-javascript"		} ,
    { ".kar"		, "audio/midi"				} ,
    { ".latex"		, "application/x-latex"			} ,
    { ".lha"		, "application/octet-stream"		} ,
    { ".lzh"		, "application/octet-stream"		} ,
    { ".m3u"		, "audio/x-mpegurl"			} ,
    { ".m4v"		, "video/x-m4v"				} ,
    { ".man"		, "application/x-troff-man"		} ,
    { ".mathml"		, "application/mathml+xml"		} ,
    { ".me"		, "application/x-troff-me"		} ,
    { ".mesh"		, "model/mesh"				} ,
    { ".mid"		, "audio/midi"				} ,
    { ".midi"		, "audio/midi"				} ,
    { ".mif"		, "application/vnd.mif"			} ,
    { ".mov"		, "video/quicktime"			} ,
    { ".movie"		, "video/x-sgi-movie"			} ,
    { ".mp2"		, "audio/mpeg"				} ,
    { ".mp3"		, "audio/mpeg"				} ,
    { ".mp4"		, "video/mp4"				} ,
    { ".mpe"		, "video/mpeg"				} ,
    { ".mpeg"		, "video/mpeg"				} ,
    { ".mpg"		, "video/mpeg"				} ,
    { ".mpga"		, "audio/mpeg"				} ,
    { ".ms"		, "application/x-troff-ms"		} ,
    { ".msh"		, "model/mesh"				} ,
    { ".mxu"		, "video/vnd.mpegurl"			} ,
    { ".nc"		, "application/x-netcdf"		} ,
    { ".oda"		, "application/oda"			} ,
    { ".ogg"		, "application/ogg"			} ,
    { ".pbm"		, "image/x-portable-bitmap"		} ,
    { ".pdb"		, "chemical/x-pdb"			} ,
    { ".pdf"		, "application/pdf"			} ,
    { ".pgm"		, "image/x-portable-graymap"		} ,
    { ".pgn"		, "application/x-chess-pgn"		} ,
    { ".png"		, "image/png"				} ,
    { ".pnm"		, "image/x-portable-anymap"		} ,
    { ".ppm"		, "image/x-portable-pixmap"		} ,
    { ".ppt"		, "application/vnd.ms-powerpoint"	} ,
    { ".ps"		, "application/postscript"		} ,
    { ".qt"		, "video/quicktime"			} ,
    { ".ra"		, "audio/x-realaudio"			} ,
    { ".ram"		, "audio/x-pn-realaudio"		} ,
    { ".ras"		, "image/x-cmu-raster"			} ,
    { ".rdf"		, "application/rdf+xml"			} ,
    { ".rgb"		, "image/x-rgb"				} ,
    { ".rm"		, "audio/x-pn-realaudio"		} ,
    { ".roff"		, "application/x-troff"			} ,
    { ".rpm"		, "audio/x-pn-realaudio-plugin"		} ,
    { ".rtf"		, "text/rtf"				} ,
    { ".rtx"		, "text/richtext"			} ,
    { ".sgm"		, "text/sgml"				} ,
    { ".sgml"		, "text/sgml"				} ,
    { ".sh"		, "application/x-sh"			} ,
    { ".shar"		, "application/x-shar"			} ,
    { ".silo"		, "model/mesh"				} ,
    { ".sit"		, "application/x-stuffit"		} ,
    { ".skd"		, "application/x-koan"			} ,
    { ".skm"		, "application/x-koan"			} ,
    { ".skp"		, "application/x-koan"			} ,
    { ".skt"		, "application/x-koan"			} ,
    { ".smi"		, "application/smil"			} ,
    { ".smil"		, "application/smil"			} ,
    { ".snd"		, "audio/basic"				} ,
    { ".so"		, "application/octet-stream"		} ,
    { ".spl"		, "application/x-futuresplash"		} ,
    { ".src"		, "application/x-wais-source"		} ,
    { ".sv4cpio"	, "application/x-sv4cpio"		} ,
    { ".sv4crc"		, "application/x-sv4crc"		} ,
    { ".svg"		, "image/svg+xml"			} ,
    { ".swf"		, "application/x-shockwave-flash"	} ,
    { ".t"		, "application/x-troff"			} ,
    { ".tar"		, "application/x-tar"			} ,
    { ".tcl"		, "application/x-tcl"			} ,
    { ".tex"		, "application/x-tex"			} ,
    { ".texi"		, "application/x-texinfo"		} ,
    { ".texinfo"	, "application/x-texinfo"		} ,
    { ".tif"		, "image/tiff"				} ,
    { ".tiff"		, "image/tiff"				} ,
    { ".tr"		, "application/x-troff"			} ,
    { ".tsv"		, "text/tab-separated-values"		} ,
    { ".txt"		, "text/plain"				} ,
    { ".ustar"		, "application/x-ustar"			} ,
    { ".vcd"		, "application/x-cdlink"		} ,
    { ".vrml"		, "model/vrml"				} ,
    { ".vxml"		, "application/voicexml+xml"		} ,
    { ".wav"		, "audio/x-wav"				} ,
    { ".wbmp"		, "image/vnd.wap.wbmp"			} ,
    { ".wbxml"		, "application/vnd.wap.wbxml"		} ,
    { ".wml"		, "text/vnd.wap.wml"			} ,
    { ".wmlc"		, "application/vnd.wap.wmlc"		} ,
    { ".wmls"		, "text/vnd.wap.wmlscript"		} ,
    { ".wmlsc"		, "application/vnd.wap.wmlscriptc"	} ,
    { ".wrl"		, "model/vrml"				} ,
    { ".x-html"		, "text/x-html" 			} ,
    { ".xbm"		, "image/x-xbitmap"			} ,
    { ".xht"		, "application/xhtml+xml"		} ,
    { ".xhtml"		, "application/xhtml+xml"		} ,
    { ".xls"		, "application/vnd.ms-excel"		} ,
    { ".xml"		, "application/xml"			} ,
    { ".xpm"		, "image/x-xpixmap"			} ,
    { ".xsl"		, "application/xml"			} ,
    { ".xslt"		, "application/xslt+xml"		} ,
    { ".xul"		, "application/vnd.mozilla.xul+xml"	} ,
    { ".xwd"		, "image/x-xwindowdump"			} ,
    { ".xyz"		, "chemical/x-xyz"			} ,
    { ".zip"		, "application/zip"			} ,
  };
  
  const struct dstring *v;
  const char           *ext;
  
  assert(filename != NULL);
  
  ext = strrchr(filename,'.');

  if (ext != NULL)
  {
    v = bsearch(
                 ext,
                 types,
                 sizeof(types)/sizeof(struct dstring),
                 sizeof(struct dstring),
                 dstring_cmp
                );
    if (v != NULL)
      return v->s2;
  }
    
  return("text/plain");
}

/******************************************************************/

static int display_file(FILE *out,tumbler__s *spec)
{
  char fname[FILENAME_MAX];
  
  assert(out  != NULL);
  assert(spec != NULL);

  snprintf(
      fname,
      sizeof(fname),
      "%4d/%02d/%02d/%s",
      spec->start.year,
      spec->start.month,
      spec->start.day,
      spec->filename
  );
    
  if (gd.cgi)
  {
    struct stat  status;
    FILE        *in;
    const char  *type;
    int          rc;
    
    rc = stat(fname,&status);
    if (rc == -1) 
    {
      if (errno == ENOENT)
        (*gd.req->error)(gd.req,HTTP_NOTFOUND,"%s: %s",fname,strerror(errno));
      else if (errno == EACCES)
        (*gd.req->error)(gd.req,HTTP_FORBIDDEN,"%s: %s",fname,strerror(errno));
      else
        (*gd.req->error)(gd.req,HTTP_ISERVERERR,"%s: %s",fname,strerror(errno));
      return(1);
    }

    in = fopen(fname,"r");
    if (in == NULL)
    {
      (*gd.req->error)(gd.req,HTTP_NOTFOUND,"%s: some internal error",fname);
      return(1);
    }
    
    type = mime_type(spec->filename);

    if (strcmp(type,"text/x-html") == 0)
    {
      gd.htmldump = in;
      fputs("Status: 200\r\nContent-type: text/html\r\n\r\n",out);
      generic_cb("main",out,NULL);
    }
    else
    {
      fprintf(
      	out,
      	"Status: 200\r\n"
      	"Content-Type: %s\r\n"
      	"Content-Length: %lu\r\n"
      	"\r\n",
      	type,
      	(unsigned long)status.st_size
      );
      
      fcopy(out,in);
    }
    fclose(in);
  }
  else
    fprintf(out,"File to open: %s\n",fname);

  return(0);
}

/*****************************************************************/

static char *tag_collect(List *list)
{
  BlogEntry entry;
  
  /*-----------------------------------------------------------------------
  ; this function used to collect all the class tags from all the entries,
  ; but I think any advertising that's placed near the top of the page will
  ; do better if it's based off the first entry to be displayed.
  ;-----------------------------------------------------------------------*/
  
  assert(list != NULL);
  
  entry = (BlogEntry)ListGetHead(list);
  
  if (!NodeValid(&entry->node) || empty_string(entry->class))
    return strdup(c_adtag);
  else
    return strdup(entry->class);
}

/********************************************************************/

static char *tag_pick(const char *tag)
{
  String *pool;
  size_t  num;
  size_t  r;
  char   *pick;
  
  assert(tag != NULL);

  if (empty_string(tag))
    return(strdup(c_adtag));
  
  pool = tag_split(&num,tag);

  /*------------------------------------------------------------------------
  ; if num is 0, then the tag string was malformed (basically, started with
  ; a ',') and therefore, we fall back to the default adtag.
  ;-----------------------------------------------------------------------*/

  if (num)
  {
    r  = (((double)rand() / (double)RAND_MAX) * (double)num); 
    assert(r < num);
    pick = fromstring(pool[r]);
  }
  else
    pick = strdup(c_adtag);
  
  free(pool);
  return(pick);
}
 
/******************************************************************/

static void free_entries(List *list)
{
  BlogEntry entry;
  
  for
  (
    entry = (BlogEntry)ListRemHead(list);
    NodeValid(&entry->node);
    entry = (BlogEntry)ListRemHead(list)
  )
  {
    assert(entry->valid);
    BlogEntryFree(entry);
  }
}

/******************************************************************/

