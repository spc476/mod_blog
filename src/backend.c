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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>
#include <syslog.h>

#include <cgilib6/util.h>

#include "blogutil.h"
#include "frontend.h"
#include "backend.h"
#include "globals.h"

/*****************************************************************/

static struct btm  calculate_previous (Blog const *,Request *,struct btm const,unit__e);
static struct btm  calculate_next     (Blog const *,Request *,struct btm const,unit__e);
static char const *mime_type          (char const *);
static int         display_file       (tumbler__s const *,Blog const *,Request *,int (*)(int,char const *,...));
static char       *tag_collect        (List *,char const *);
static char       *tag_pick           (char const *,char const *);
static void        free_entries       (List *);

/************************************************************************/

struct callback_data *callback_init(struct callback_data *cbd,Blog const *blog,Request const *request)
{
  assert(cbd     != NULL);
  assert(blog    != NULL);
  assert(request != NULL);
  
  memset(cbd,0,sizeof(struct callback_data));
  ListInit(&cbd->list);
  cbd->template = &blog->config.templates[0]; /* XXX probably document this */
  cbd->request  = request;
  cbd->blog     = blog;
  cbd->entry    = NULL;
  cbd->ad       = NULL;
  cbd->adtag    = NULL;
  cbd->adcat    = NULL;
  cbd->navunit  = UNIT_PART;
  return cbd;
}

/************************************************************************/

pagegen__f TO_pagegen(char const *name)
{
  assert(name != NULL);
  if (strcmp(name,"items") == 0)
    return pagegen_items;
  else if (strcmp(name,"days") == 0)
    return pagegen_days;
  else
  {
    assert(0);
    return pagegen_items;
  }
}

/************************************************************************/

int generate_thisday(FILE *out,struct btm when,Blog const *blog,Request *request)
{
  struct callback_data  cbd;
  char                 *tags;
  
  assert(out     != NULL);
  assert(blog    != NULL);
  assert(request != NULL);
  
  callback_init(&cbd,blog,request);
  
  for(when.year = blog->first.year ; when.year <= blog->now.year ; when.year++)
  {
    if (btm_cmp_date(&when,&blog->first) < 0)
      continue;
      
    for (when.part = 1 ; ; when.part++)
    {
      BlogEntry *entry = BlogEntryRead(blog,&when);
      if (entry)
      {
        assert(entry->valid);
        ListAddTail(&cbd.list,&entry->node);
      }
      else
        break;
    }
  }
  
  tags = tag_collect(&cbd.list,blog->config.adtag);
  cbd.adtag = tag_pick(tags,blog->config.adtag);
  free(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);
  free(cbd.adtag);
  return 0;
}

/************************************************************************/

int generate_pages(Blog const *blog,Request *request)
{
  assert(blog    != NULL);
  assert(request != NULL);
  (void)request;
    
  for (size_t i = 0 ; i < blog->config.templatenum ; i++)
  {
    FILE  *out = fopen(blog->config.templates[i].file,"w");
    int  (*pagegen)(struct template const *,FILE *,Blog const *,Request *);
    
    if (out == NULL)
    {
      syslog(LOG_ERR,"%s: %s",blog->config.templates[i].file,strerror(errno));
      continue;
    }
    
    pagegen = TO_pagegen(blog->config.templates[i].pagegen);
    (*pagegen)(&blog->config.templates[i],out,blog,request);
    fclose(out);
    
    if (blog->config.templates[i].posthook)
    {
      char const *argv[3];
      
      argv[0] = blog->config.templates[i].posthook;
      argv[1] = blog->config.templates[i].file;
      argv[2] = NULL;
      
      run_hook("template-post-hook",argv);
    }
  }
  
  return 0;
}

/******************************************************************/

int pagegen_items(
        template__t const *template,
        FILE              *out,
        Blog        const *blog,
        Request           *request
)
{
  struct btm            thisday;
  char                 *tags;
  struct callback_data  cbd;
  
  assert(template != NULL);
  assert(out      != NULL);
  assert(blog     != NULL);
  assert(request  != NULL);
  
  request->f.fullurl = template->fullurl;
  request->f.reverse = template->reverse;
  thisday      = blog->now;
  
  callback_init(&cbd,blog,request);
  cbd.template = template;
  
  if (template->reverse)
    BlogEntryReadXD(blog,&cbd.list,&thisday,template->items);
  else
    BlogEntryReadXU(blog,&cbd.list,&thisday,template->items);
    
  tags      = tag_collect(&cbd.list,blog->config.adtag);
  cbd.adtag = tag_pick(tags,blog->config.adtag);
  
  free(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);
  free(cbd.adtag);
  return 0;
}

/************************************************************************/

int pagegen_days(
        template__t const *template,
        FILE              *out,
        Blog        const *blog,
        Request           *request
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
  assert(request  != NULL);
  
  request->f.fullurl = false;
  request->f.reverse = true;
  thisday      = blog->now;
  
  callback_init(&cbd,blog,request);
  cbd.template = template;
  
  for (days = 0 , added = false ; days < template->items ; )
  {
    BlogEntry *entry;
    
    if (btm_cmp(&thisday,&blog->first) < 0) break;
    
    entry = BlogEntryRead(blog,&thisday);
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
  
  tags      = tag_collect(&cbd.list,blog->config.adtag);
  cbd.adtag = tag_pick(tags,blog->config.adtag);
  
  free(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);
  free(cbd.adtag);
  return 0;
}

/************************************************************************/

static void swap_endpoints(tumbler__s *tum)
{
  struct btm start;
  struct btm stop;
  
  assert(tum         != NULL);
  assert(tum->ustart != UNIT_YEAR);
  assert(tum->ustop  != UNIT_YEAR);
  
  /*--------------------------------------------------------------------
  ; The best way to explain this is to give a bunch of examples.
  ;     M = month D = day P = part
  ;
  ; 2000/12-09                  MM
  ;     2000/12/01.1 2000/09/30.23      2000/09/01.1 2000/12/31.23
  ;
  ; 2000/12-09/15               MD
  ;     2000/12/01.1 2000/09/15.23      2000/09/15.1 2000/12/31.23
  ;
  ; 2000/12-09/15.2             MP
  ;     2000/12/01.1 2000/09/15.2       2000/09/15.2 2000/12/31.23
  ;
  ; 2000/12/20-2000/09          DM
  ;     2000/12/20.1 2000/09/30.23      2000/09/01.1 2000/12/20.23
  ;
  ; 2000/12/20-09/15            DD
  ;     2000/12/20.1 2000/09/15.23      2000/09/15.1 2000/12/20.23
  ;
  ; 2000/12/20-09/15.2          DP
  ;     2000/12/20.1 2000/09/15.2       2000/09/15.2 2000/12/20.23
  ;
  ; 2000/12/20.2-2000/09        PM
  ;     2000/12/20.2 2000/09/30.23      2000/09/01.1 2000/12/20.2
  ;
  ; 2000/12/20.2-09/15          PD
  ;     2000/12/20.2 2000/09/15.23      2000/09/15.1 2000/12/20.2
  ;
  ; 2000/12/20.2-09/15.3        PP
  ;     2000/12/20.2 2000/09/15.3       2000/09/15.3 2000/12/20.2
  ;
  ; The year and the month are always swapped.
  ;--------------------------------------------------------------------*/
  
  start.year  = tum->stop.year;
  start.month = tum->stop.month;
  stop.year   = tum->start.year;
  stop.month  = tum->start.month;
  
  switch(tum->ustart)
  {
    case UNIT_MONTH:
         stop.day  = max_monthday(stop.year,stop.month);
         stop.part = ENTRY_MAX;
         break;
         
    case UNIT_DAY:
         stop.day  = tum->start.day;
         stop.part = ENTRY_MAX;
         break;
         
    case UNIT_PART:
         stop.day  = tum->start.day;
         stop.part = tum->start.part;
         break;
         
    case UNIT_YEAR:
         assert(0);
         break;
  }
  
  switch(tum->ustop)
  {
    case UNIT_MONTH:
         start.day  = 1;
         start.part = 1;
         break;
         
    case UNIT_DAY:
         start.day  = tum->stop.day;
         start.part = 1;
         break;
         
    case UNIT_PART:
         start.day  = tum->stop.day;
         start.part = tum->stop.part;
         break;
         
    case UNIT_YEAR:
         assert(0);
         break;
  }
  
  tum->start = start;
  tum->stop  = stop;
}

/************************************************************************/

int tumbler_page(tumbler__s *spec,Blog const *blog,Request *request,int (*errorf)(int,char const *,...))
{
  struct callback_data cbd;
  struct btm           start;
  struct btm           end;
  char                *tags;
  
  assert(spec    != NULL);
  assert(blog    != NULL);
  assert(request != NULL);
  assert(errorf);
  
  request->f.fullurl = false; /* XXX why? */
  request->f.navprev = true;
  request->f.navnext = true;
  
  if (spec->redirect)
    return HTTP_NOTIMP;
    
  if (spec->file)
  {
    display_file(spec,blog,request,errorf);
    return 0; /* XXX hack for now */
  }
  
  callback_init(&cbd,blog,request);
  
  /*----------------------------------------------------------------------
  ; from here to the comment about sanity checking replaced around 100 very
  ; confused lines of code.  The tumbler code now does more checking of data
  ; than the old version, so a lot of code was removed.
  ;-----------------------------------------------------------------------*/
  
  assert(spec->start.year  >= blog->first.year);
  assert(spec->start.month >   0);
  assert(spec->start.month <  13);
  assert(spec->start.day   >   0);
  assert(spec->start.day   <= max_monthday(spec->start.year,spec->start.month));
  
  assert(spec->stop.year  >= blog->first.year);
  assert(spec->stop.month >   0);
  assert(spec->stop.month <  13);
  assert(spec->stop.day   >   0);
  assert(spec->stop.day   <= max_monthday(spec->stop.year,spec->stop.month));
  
  start = spec->start;
  end   = spec->stop;
  
  if (!spec->range)
  {
    request->f.navigation  = true;
    cbd.navunit      = spec->ustart > spec->ustop
                     ? spec->ustart
                     : spec->ustop
                     ;
    cbd.previous = calculate_previous(blog,request,start,cbd.navunit);
    cbd.next     = calculate_next(blog,request,end,cbd.navunit);
  }
  else
  {
    if (btm_cmp(&spec->start,&spec->stop) > 0)
    {
      tumbler__s newtum = *spec;
      swap_endpoints(&newtum);
      start        = newtum.start;
      end          = newtum.stop;
      request->f.reverse = true; /* XXX */
    }
  }
  
  assert(end.day <= max_monthday(end.year,end.month));
  
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
  
  assert(btm_cmp(&start,&end) <= 0);
  
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
  
  if (request->f.reverse)
    BlogEntryReadBetweenD(blog,&cbd.list,&end,&start);
  else
    BlogEntryReadBetweenU(blog,&cbd.list,&start,&end);
    
  tags      = tag_collect(&cbd.list,blog->config.adtag);
  cbd.adtag = tag_pick(tags,blog->config.adtag);
  
  free(tags);
  generic_cb("main",stdout,&cbd);
  free_entries(&cbd.list);
  free(cbd.adtag);
  return 0;
}

/******************************************************************/

static struct btm calculate_previous(Blog const *blog,Request *request,struct btm const start,unit__e navunit)
{
  struct btm previous = start;
  
  switch(navunit)
  {
    case UNIT_YEAR:
         if (start.year == blog->first.year)
           request->f.navprev = false;
         else
           previous.year = start.year - 1;
         break;
         
    case UNIT_MONTH:
         if (
              (start.year == blog->first.year)
              && (start.month == blog->first.month)
            )
           request->f.navprev = false;
         else
         {
           btm_dec_month(&previous);
           previous.day  = max_monthday(previous.year,previous.month);
           previous.part = 1;
           
           while(btm_cmp(&previous,&blog->first) >= 0)
           {
             BlogEntry *entry = BlogEntryRead(blog,&previous);
             if (entry == NULL)
             {
               btm_dec_day(&previous);
               continue;
             }
             
             assert(entry->valid);
             BlogEntryFree(entry);
             return previous;
           }
           
           request->f.navprev = false;
         }
         break;
         
    case UNIT_DAY:
         if (btm_cmp_date(&start,&blog->first) == 0)
           request->f.navprev = false;
         else
         {
           btm_dec_day(&previous);
           previous.part = 1;
           
           while(btm_cmp(&previous,&blog->first) >= 0)
           {
             BlogEntry *entry = BlogEntryRead(blog,&previous);
             if (entry == NULL)
             {
               btm_dec_day(&previous);
               continue;
             }
             
             assert(entry->valid);
             BlogEntryFree(entry);
             return previous;
           }
           
           request->f.navprev = false;
         }
         break;
         
    case UNIT_PART:
         if (btm_cmp(&start,&blog->first) == 0)
           request->f.navprev = false;
         else
         {
           btm_dec_part(&previous);
           
           while(btm_cmp(&previous,&blog->first) >= 0)
           {
             BlogEntry *entry = BlogEntryRead(blog,&previous);
             if (entry == NULL)
             {
               btm_dec_part(&previous);
               continue;
             }
             
             assert(entry->valid);
             BlogEntryFree(entry);
             return previous;
           }
           
           request->f.navprev = false;
         }
         break;
         
    default:
         assert(0);
  }
  
  return previous;
}

/******************************************************************/

static struct btm calculate_next(Blog const *blog,Request *request,struct btm const end,unit__e navunit)
{
  struct btm next = end;
  
  switch(navunit)
  {
    case UNIT_YEAR:
         if (end.year == blog->now.year)
           request->f.navnext = false;
         else
           next.year  = end.year + 1;
         break;
         
    case UNIT_MONTH:
         if (
              (end.year == blog->now.year)
              && (end.month == blog->now.month)
            )
           request->f.navnext = false;
         else
         {
           btm_inc_month(&next);
           next.day  = 1;
           next.part = 1;
           
           while(btm_cmp(&next,&blog->now) <= 0)
           {
             BlogEntry *entry = BlogEntryRead(blog,&next);
             if (entry == NULL)
             {
               btm_inc_day(&next);
               continue;
             }
             
             assert(entry->valid);
             BlogEntryFree(entry);
             return next;
           }
           
           request->f.navnext = false;
         }
         break;
         
    case UNIT_DAY:
         if (btm_cmp_date(&end,&blog->now) == 0)
           request->f.navnext = false;
         else
         {
           btm_inc_day(&next);
           next.part = 1;
           
           while(btm_cmp(&next,&blog->now) <= 0)
           {
             BlogEntry *entry = BlogEntryRead(blog,&next);
             if (entry == NULL)
             {
               btm_inc_day(&next);
               continue;
             }
             
             assert(entry->valid);
             BlogEntryFree(entry);
             return next;
           }
           
           request->f.navnext = false;
         }
         break;
         
    case UNIT_PART:
         if (btm_cmp(&end,&blog->now) == 0)
           request->f.navnext = false;
         else
         {
           next.part++;
           
           while(btm_cmp(&next,&blog->now) <= 0)
           {
             BlogEntry *entry = BlogEntryRead(blog,&next);
             if (entry == NULL)
             {
               next.part = 1;
               btm_inc_day(&next);
               continue;
             }
             
             assert(entry->valid);
             BlogEntryFree(entry);
             return next;
           }
           request->f.navnext = false;
         }
         break;
         
    default:
         assert(0);
  }
  
  return next;
}

/******************************************************************/

static int dstring_cmp(void const *needle,void const *haystack)
{
  char const           *key   = needle;
  struct dstring const *value = haystack;
  
  assert(needle   != NULL);
  assert(haystack != NULL);
  
  return strcmp(key,value->s1);
}

static char const *mime_type(char const *filename)
{
  static struct dstring const types[] =
  {
    { ".ai"             , "application/postscript"              } ,
    { ".aif"            , "audio/x-aiff"                        } ,
    { ".aifc"           , "audio/x-aiff"                        } ,
    { ".aiff"           , "audio/x-aiff"                        } ,
    { ".asc"            , "text/plain"                          } ,
    { ".au"             , "audio/basic"                         } ,
    { ".avi"            , "video/x-msvideo"                     } ,
    { ".bcpio"          , "application/x-bcpio"                 } ,
    { ".bin"            , "application/octet-stream"            } ,
    { ".bmp"            , "image/bmp"                           } ,
    { ".cdf"            , "application/x-netcdf"                } ,
    { ".cgm"            , "image/cgm"                           } ,
    { ".class"          , "application/octet-stream"            } ,
    { ".cpio"           , "application/x-cpio"                  } ,
    { ".cpt"            , "application/mac-compactpro"          } ,
    { ".csh"            , "application/x-csh"                   } ,
    { ".css"            , "text/css"                            } ,
    { ".dcr"            , "application/x-director"              } ,
    { ".dir"            , "application/x-director"              } ,
    { ".djv"            , "image/vnd.djvu"                      } ,
    { ".djvu"           , "image/vnd.djvu"                      } ,
    { ".dll"            , "application/octet-stream"            } ,
    { ".dms"            , "application/octet-stream"            } ,
    { ".doc"            , "application/msword"                  } ,
    { ".dtd"            , "application/xml-dtd"                 } ,
    { ".dvi"            , "application/x-dvi"                   } ,
    { ".dxr"            , "application/x-director"              } ,
    { ".eps"            , "application/postscript"              } ,
    { ".etx"            , "text/x-setext"                       } ,
    { ".exe"            , "application/octet-stream"            } ,
    { ".ez"             , "application/andrew-inset"            } ,
    { ".gif"            , "image/gif"                           } ,
    { ".gram"           , "application/srgs"                    } ,
    { ".grxml"          , "application/srgs+xml"                } ,
    { ".gtar"           , "application/x-gtar"                  } ,
    { ".gz"             , "application/x-gzip"                  } ,
    { ".hdf"            , "application/x-hdf"                   } ,
    { ".hqx"            , "application/mac-binhex40"            } ,
    { ".htm"            , "text/html"                           } ,
    { ".html"           , "text/html"                           } ,
    { ".ice"            , "x-conference/x-cooltalk"             } ,
    { ".ico"            , "image/x-icon"                        } ,
    { ".ics"            , "text/calendar"                       } ,
    { ".ief"            , "image/ief"                           } ,
    { ".ifb"            , "text/calendar"                       } ,
    { ".iges"           , "model/iges"                          } ,
    { ".igs"            , "model/iges"                          } ,
    { ".jpe"            , "image/jpeg"                          } ,
    { ".jpeg"           , "image/jpeg"                          } ,
    { ".jpg"            , "image/jpeg"                          } ,
    { ".js"             , "application/x-javascript"            } ,
    { ".kar"            , "audio/midi"                          } ,
    { ".latex"          , "application/x-latex"                 } ,
    { ".lha"            , "application/octet-stream"            } ,
    { ".lzh"            , "application/octet-stream"            } ,
    { ".m3u"            , "audio/x-mpegurl"                     } ,
    { ".m4v"            , "video/x-m4v"                         } ,
    { ".man"            , "application/x-troff-man"             } ,
    { ".mathml"         , "application/mathml+xml"              } ,
    { ".me"             , "application/x-troff-me"              } ,
    { ".mesh"           , "model/mesh"                          } ,
    { ".mid"            , "audio/midi"                          } ,
    { ".midi"           , "audio/midi"                          } ,
    { ".mif"            , "application/vnd.mif"                 } ,
    { ".mov"            , "video/quicktime"                     } ,
    { ".movie"          , "video/x-sgi-movie"                   } ,
    { ".mp2"            , "audio/mpeg"                          } ,
    { ".mp3"            , "audio/mpeg"                          } ,
    { ".mp4"            , "video/mp4"                           } ,
    { ".mpe"            , "video/mpeg"                          } ,
    { ".mpeg"           , "video/mpeg"                          } ,
    { ".mpg"            , "video/mpeg"                          } ,
    { ".mpga"           , "audio/mpeg"                          } ,
    { ".ms"             , "application/x-troff-ms"              } ,
    { ".msh"            , "model/mesh"                          } ,
    { ".mxu"            , "video/vnd.mpegurl"                   } ,
    { ".nc"             , "application/x-netcdf"                } ,
    { ".oda"            , "application/oda"                     } ,
    { ".ogg"            , "application/ogg"                     } ,
    { ".pbm"            , "image/x-portable-bitmap"             } ,
    { ".pdb"            , "chemical/x-pdb"                      } ,
    { ".pdf"            , "application/pdf"                     } ,
    { ".pgm"            , "image/x-portable-graymap"            } ,
    { ".pgn"            , "application/x-chess-pgn"             } ,
    { ".png"            , "image/png"                           } ,
    { ".pnm"            , "image/x-portable-anymap"             } ,
    { ".ppm"            , "image/x-portable-pixmap"             } ,
    { ".ppt"            , "application/vnd.ms-powerpoint"       } ,
    { ".ps"             , "application/postscript"              } ,
    { ".qt"             , "video/quicktime"                     } ,
    { ".ra"             , "audio/x-realaudio"                   } ,
    { ".ram"            , "audio/x-pn-realaudio"                } ,
    { ".ras"            , "image/x-cmu-raster"                  } ,
    { ".rdf"            , "application/rdf+xml"                 } ,
    { ".rgb"            , "image/x-rgb"                         } ,
    { ".rm"             , "audio/x-pn-realaudio"                } ,
    { ".roff"           , "application/x-troff"                 } ,
    { ".rpm"            , "audio/x-pn-realaudio-plugin"         } ,
    { ".rtf"            , "text/rtf"                            } ,
    { ".rtx"            , "text/richtext"                       } ,
    { ".sgm"            , "text/sgml"                           } ,
    { ".sgml"           , "text/sgml"                           } ,
    { ".sh"             , "application/x-sh"                    } ,
    { ".shar"           , "application/x-shar"                  } ,
    { ".silo"           , "model/mesh"                          } ,
    { ".sit"            , "application/x-stuffit"               } ,
    { ".skd"            , "application/x-koan"                  } ,
    { ".skm"            , "application/x-koan"                  } ,
    { ".skp"            , "application/x-koan"                  } ,
    { ".skt"            , "application/x-koan"                  } ,
    { ".smi"            , "application/smil"                    } ,
    { ".smil"           , "application/smil"                    } ,
    { ".snd"            , "audio/basic"                         } ,
    { ".so"             , "application/octet-stream"            } ,
    { ".spl"            , "application/x-futuresplash"          } ,
    { ".src"            , "application/x-wais-source"           } ,
    { ".sv4cpio"        , "application/x-sv4cpio"               } ,
    { ".sv4crc"         , "application/x-sv4crc"                } ,
    { ".svg"            , "image/svg+xml"                       } ,
    { ".swf"            , "application/x-shockwave-flash"       } ,
    { ".t"              , "application/x-troff"                 } ,
    { ".tar"            , "application/x-tar"                   } ,
    { ".tcl"            , "application/x-tcl"                   } ,
    { ".tex"            , "application/x-tex"                   } ,
    { ".texi"           , "application/x-texinfo"               } ,
    { ".texinfo"        , "application/x-texinfo"               } ,
    { ".tif"            , "image/tiff"                          } ,
    { ".tiff"           , "image/tiff"                          } ,
    { ".tr"             , "application/x-troff"                 } ,
    { ".tsv"            , "text/tab-separated-values"           } ,
    { ".txt"            , "text/plain"                          } ,
    { ".ustar"          , "application/x-ustar"                 } ,
    { ".vcd"            , "application/x-cdlink"                } ,
    { ".vrml"           , "model/vrml"                          } ,
    { ".vxml"           , "application/voicexml+xml"            } ,
    { ".wav"            , "audio/x-wav"                         } ,
    { ".wbmp"           , "image/vnd.wap.wbmp"                  } ,
    { ".wbxml"          , "application/vnd.wap.wbxml"           } ,
    { ".wml"            , "text/vnd.wap.wml"                    } ,
    { ".wmlc"           , "application/vnd.wap.wmlc"            } ,
    { ".wmls"           , "text/vnd.wap.wmlscript"              } ,
    { ".wmlsc"          , "application/vnd.wap.wmlscriptc"      } ,
    { ".wrl"            , "model/vrml"                          } ,
    { ".x-html"         , "text/x-html"                         } ,
    { ".xbm"            , "image/x-xbitmap"                     } ,
    { ".xht"            , "application/xhtml+xml"               } ,
    { ".xhtml"          , "application/xhtml+xml"               } ,
    { ".xls"            , "application/vnd.ms-excel"            } ,
    { ".xml"            , "application/xml"                     } ,
    { ".xpm"            , "image/x-xpixmap"                     } ,
    { ".xsl"            , "application/xml"                     } ,
    { ".xslt"           , "application/xslt+xml"                } ,
    { ".xul"            , "application/vnd.mozilla.xul+xml"     } ,
    { ".xwd"            , "image/x-xwindowdump"                 } ,
    { ".xyz"            , "chemical/x-xyz"                      } ,
    { ".zip"            , "application/zip"                     } ,
  };
  
  char const           *ext;
  
  assert(filename != NULL);
  
  ext = strrchr(filename,'.');
  
  if (ext != NULL)
  {
    struct dstring const *v = bsearch(
                 ext,
                 types,
                 sizeof(types)/sizeof(struct dstring),
                 sizeof(struct dstring),
                 dstring_cmp
                );
    if (v != NULL)
      return v->s2;
  }
  
  return "text/plain";
}

/******************************************************************/

static int display_file(tumbler__s const *spec,Blog const *blog,Request *request,int (*errorf)(int,char const *,...))
{
  char fname[FILENAME_MAX];
  
  assert(spec != NULL);
  assert(errorf);
  
  snprintf(
      fname,
      sizeof(fname),
      "%4d/%02d/%02d/%s",
      spec->start.year,
      spec->start.month,
      spec->start.day,
      spec->filename
  );
  
  if (request->f.cgi)
  {
    struct stat  status;
    char const  *type;
    int          rc;
    
    rc = stat(fname,&status);
    if (rc == -1)
    {
      if (errno == ENOENT)
        (*errorf)(HTTP_NOTFOUND,"%s: %s",fname,strerror(errno));
      else if (errno == EACCES)
        (*errorf)(HTTP_FORBIDDEN,"%s: %s",fname,strerror(errno));
      else
        (*errorf)(HTTP_ISERVERERR,"%s: %s",fname,strerror(errno));
      return 1;
    }
    
    if (freopen(fname,"r",stdin) == NULL)
    {
      (*errorf)(HTTP_NOTFOUND,"%s: some internal error",fname);
      return 1;
    }
    
    type = mime_type(spec->filename);
    
    if (strcmp(type,"text/x-html") == 0)
    {
      struct callback_data cbd;
      
      request->f.htmldump = true;
      fputs("Status: 200\r\nContent-type: text/html\r\n\r\n",stdout);
      generic_cb("main",stdout,callback_init(&cbd,blog,request));
    }
    else
    {
      fprintf(
        stdout,
        "Status: 200\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lu\r\n"
        "\r\n",
        type,
        (unsigned long)status.st_size
      );
      
      fcopy(stdout,stdin);
    }
    fclose(stdin);
  }
  else
    fprintf(stdout,"File to open: %s\n",fname);
    
  return 0;
}

/*****************************************************************/

static char *tag_collect(List *list,char const *def)
{
  BlogEntry *entry;
  
  /*-----------------------------------------------------------------------
  ; this function used to collect all the class tags from all the entries,
  ; but I think any advertising that's placed near the top of the page will
  ; do better if it's based off the first entry to be displayed.
  ;-----------------------------------------------------------------------*/
  
  assert(list != NULL);
  assert(def  != NULL);
  
  entry = (BlogEntry *)ListGetHead(list);
  
  if (NodeValid(&entry->node))
  {
    if (!empty_string(entry->adtag))
      return strdup(entry->adtag);
      
    if (!empty_string(entry->class))
      return strdup(entry->class);
  }
  
  return strdup(def);
}

/********************************************************************/

static char *tag_pick(char const *tag,char const *def)
{
  String *pool;
  size_t  num;
  char   *pick;
  
  assert(tag != NULL);
  assert(def != NULL);
  
  if (empty_string(tag))
    return strdup(def);
    
  pool = tag_split(&num,tag);
  
  /*------------------------------------------------------------------------
  ; if num is 0, then the tag string was malformed (basically, started with
  ; a ',') and therefore, we fall back to the default adtag.
  ;-----------------------------------------------------------------------*/
  
  if (num)
  {
    size_t r = (((double)rand() / (double)RAND_MAX) * (double)num);
    assert(r < num);
    pick     = fromstring(pool[r]);
  }
  else
    pick = strdup(def);
    
  free(pool);
  return pick;
}

/******************************************************************/

static void free_entries(List *list)
{
  assert(list != NULL);
  
  for
  (
    BlogEntry *entry = (BlogEntry *)ListRemHead(list);
    NodeValid(&entry->node);
    entry = (BlogEntry *)ListRemHead(list)
  )
  {
    assert(entry->valid);
    BlogEntryFree(entry);
  }
}

/******************************************************************/
