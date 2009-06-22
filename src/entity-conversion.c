/*********************************************************************
*
* Copyright 2009 by Sean Conner.  All Rights Reserved.
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

#define _GNU_SOURCE	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

/*********************************************************************/

struct entity_conv
{
  const char *const name;
  const       int   value;
};

/********************************************************************/

static bool		char_entity	(char **,size_t *,int);

static __ssize_t	fer_read	(void *,char *,size_t);
static __ssize_t	few_read	(void *,char *,size_t);
static __ssize_t	fer_write	(void *,const char *,size_t);
static __ssize_t	few_write	(void *,const char *,size_t);
static int		fnop_seek	(void *,_IO_off64_t *,int);
static int		fnop_close	(void *);

/********************************************************************/

	/*------------------------------------------------
	; the following table is sorted via the C locale.
	; This is okay, as that locale for collation is
	; set at the beginning of the program.
	;----------------------------------------------*/

const struct entity_conv m_table[] =
{
  { "AElig" 	,  198 },
  { "Aacute" 	,  193 },
  { "Acirc" 	,  194 },
  { "Agrave" 	,  192 },
  { "Alpha" 	,  913 },
  { "Aring" 	,  197 },
  { "Atilde" 	,  195 },
  { "Auml" 	,  196 },
  { "Beta" 	,  914 },
  { "Ccedil" 	,  199 },
  { "Chi" 	,  935 },
  { "Dagger" 	, 8225 },
  { "Delta" 	,  916 },
  { "ETH" 	,  208 },
  { "Eacute" 	,  201 },
  { "Ecirc" 	,  202 },
  { "Egrave" 	,  200 },
  { "Epsilon" 	,  917 },
  { "Eta" 	,  919 },
  { "Euml" 	,  203 },
  { "Gamma" 	,  915 },
  { "Iacute" 	,  205 },
  { "Icirc" 	,  206 },
  { "Igrave" 	,  204 },
  { "Iota" 	,  921 },
  { "Iuml" 	,  207 },
  { "Kappa" 	,  922 },
  { "Lambda" 	,  923 },
  { "Mu" 	,  924 },
  { "Ntilde" 	,  209 },
  { "Nu" 	,  925 },
  { "OElig" 	,  338 },
  { "Oacute" 	,  211 },
  { "Ocirc" 	,  212 },
  { "Ograve" 	,  210 },
  { "Omega" 	,  937 },
  { "Omicron" 	,  927 },
  { "Oslash" 	,  216 },
  { "Otilde" 	,  213 },
  { "Ouml" 	,  214 },
  { "Phi" 	,  934 },
  { "Pi" 	,  928 },
  { "Prime" 	, 8243 },
  { "Psi" 	,  936 },
  { "Rho" 	,  929 },
  { "Scaron" 	,  352 },
  { "Sigma" 	,  931 },
  { "THORN" 	,  222 },
  { "Tau" 	,  932 },
  { "Theta" 	,  920 },
  { "Uacute" 	,  218 },
  { "Ucirc" 	,  219 },
  { "Ugrave" 	,  217 },
  { "Upsilon" 	,  933 },
  { "Uuml" 	,  220 },
  { "Xi" 	,  926 },
  { "Yacute" 	,  221 },
  { "Yuml" 	,  376 },
  { "Zeta" 	,  918 },
  { "aacute" 	,  225 },
  { "acirc" 	,  226 },
  { "acute" 	,  180 },
  { "aelig" 	,  230 },
  { "agrave" 	,  224 },
  { "alefsym" 	, 8501 },
  { "alpha" 	,  945 },
  { "amp" 	,   38 },
  { "and" 	, 8743 },
  { "ang" 	, 8736 },
  { "aring" 	,  229 },
  { "asymp" 	, 8776 },
  { "atilde" 	,  227 },
  { "auml" 	,  228 },
  { "bdquo" 	, 8222 },
  { "beta" 	,  946 },
  { "brvbar" 	,  166 },
  { "bull" 	, 8226 },
  { "cap" 	, 8745 },
  { "ccedil" 	,  231 },
  { "cedil" 	,  184 },
  { "cent" 	,  162 },
  { "chi" 	,  967 },
  { "circ" 	,  710 },
  { "clubs" 	, 9827 },
  { "cong" 	, 8773 },
  { "copy" 	,  169 },
  { "crarr" 	, 8629 },
  { "cup" 	, 8746 },
  { "curren" 	,  164 },
  { "dArr" 	, 8659 },
  { "dagger" 	, 8224 },
  { "darr" 	, 8595 },
  { "deg" 	,  176 },
  { "delta" 	,  948 },
  { "diams" 	, 9830 },
  { "divide" 	,  247 },
  { "eacute" 	,  233 },
  { "ecirc" 	,  234 },
  { "egrave" 	,  232 },
  { "empty" 	, 8709 },
  { "emsp" 	, 8195 },
  { "ensp" 	, 8194 },
  { "epsilon" 	,  949 },
  { "equiv" 	, 8801 },
  { "eta" 	,  951 },
  { "eth" 	,  240 },
  { "euml" 	,  235 },
  { "euro" 	, 8364 },
  { "exist" 	, 8707 },
  { "fnof" 	,  402 },
  { "forall" 	, 8704 },
  { "frac12" 	,  189 },
  { "frac14" 	,  188 },
  { "frac34" 	,  190 },
  { "frasl" 	, 8260 },
  { "gamma" 	,  947 },
  { "ge" 	, 8805 },
  { "gt" 	,   62 },
  { "hArr" 	, 8660 },
  { "harr" 	, 8596 },
  { "hearts" 	, 9829 },
  { "hellip" 	, 8230 },
  { "iacute" 	,  237 },
  { "icirc" 	,  238 },
  { "iexcl" 	,  161 },
  { "igrave" 	,  236 },
  { "image" 	, 8465 },
  { "infin" 	, 8734 },
  { "int" 	, 8747 },
  { "iota" 	,  953 },
  { "iquest" 	,  191 },
  { "isin" 	, 8712 },
  { "iuml" 	,  239 },
  { "kappa" 	,  954 },
  { "lArr" 	, 8656 },
  { "lambda" 	,  955 },
  { "lang" 	, 9001 },
  { "laquo" 	,  171 },
  { "larr" 	, 8592 },
  { "lceil" 	, 8968 },
  { "ldquo" 	, 8220 },
  { "le" 	, 8804 },
  { "lfloor" 	, 8970 },
  { "lowast" 	, 8727 },
  { "loz" 	, 9674 },
  { "lrm" 	, 8206 },
  { "lsaquo" 	, 8249 },
  { "lsquo" 	, 8216 },
  { "lt" 	,   60 },
  { "macr" 	,  175 },
  { "mdash" 	, 8212 },
  { "micro" 	,  181 },
  { "middot" 	,  183 },
  { "minus" 	, 8722 },
  { "mu" 	,  956 },
  { "nabla" 	, 8711 },
  { "nbsp" 	,  160 },
  { "ndash" 	, 8211 },
  { "ne" 	, 8800 },
  { "ni" 	, 8715 },
  { "not" 	,  172 },
  { "notin" 	, 8713 },
  { "nsub" 	, 8836 },
  { "ntilde" 	,  241 },
  { "nu" 	,  957 },
  { "oacute" 	,  243 },
  { "ocirc" 	,  244 },
  { "oelig" 	,  339 },
  { "ograve" 	,  242 },
  { "oline" 	, 8254 },
  { "omega" 	,  969 },
  { "omicron" 	,  959 },
  { "oplus" 	, 8853 },
  { "or" 	, 8744 },
  { "ordf" 	,  170 },
  { "ordm" 	,  186 },
  { "oslash" 	,  248 },
  { "otilde" 	,  245 },
  { "otimes" 	, 8855 },
  { "ouml" 	,  246 },
  { "para" 	,  182 },
  { "part" 	, 8706 },
  { "permil" 	, 8240 },
  { "perp" 	, 8869 },
  { "phi" 	,  966 },
  { "pi" 	,  960 },
  { "piv" 	,  982 },
  { "plusmn" 	,  177 },
  { "pound" 	,  163 },
  { "prime" 	, 8242 },
  { "prod" 	, 8719 },
  { "prop" 	, 8733 },
  { "psi" 	,  968 },
  { "quot" 	,   34 },
  { "rArr" 	, 8658 },
  { "radic" 	, 8730 },
  { "rang" 	, 9002 },
  { "raquo" 	,  187 },
  { "rarr" 	, 8594 },
  { "rceil" 	, 8969 },
  { "rdquo" 	, 8221 },
  { "real" 	, 8476 },
  { "reg" 	,  174 },
  { "rfloor" 	, 8971 },
  { "rho" 	,  961 },
  { "rlm" 	, 8207 },
  { "rsaquo" 	, 8250 },
  { "rsquo" 	, 8217 },
  { "sbquo" 	, 8218 },
  { "scaron" 	,  353 },
  { "sdot" 	, 8901 },
  { "sect" 	,  167 },
  { "shy" 	,  173 },
  { "sigma" 	,  963 },
  { "sigmaf" 	,  962 },
  { "sim" 	, 8764 },
  { "spades" 	, 9824 },
  { "sub" 	, 8834 },
  { "sube" 	, 8838 },
  { "sum" 	, 8721 },
  { "sup" 	, 8835 },
  { "sup1" 	,  185 },
  { "sup2" 	,  178 },
  { "sup3" 	,  179 },
  { "supe" 	, 8839 },
  { "szlig" 	,  223 },
  { "tau" 	,  964 },
  { "there4" 	, 8756 },
  { "theta" 	,  952 },
  { "thetasym" 	,  977 },
  { "thinsp" 	, 8201 },
  { "thorn" 	,  254 },
  { "tilde" 	,  732 },
  { "times" 	,  215 },
  { "trade" 	, 8482 },
  { "uArr" 	, 8657 },
  { "uacute" 	,  250 },
  { "uarr" 	, 8593 },
  { "ucirc" 	,  251 },
  { "ugrave" 	,  249 },
  { "uml" 	,  168 },
  { "upsih" 	,  978 },
  { "upsilon" 	,  965 },
  { "uuml" 	,  252 },
  { "weierp" 	, 8472 },
  { "xi" 	,  958 },
  { "yacute" 	,  253 },
  { "yen" 	,  165 },
  { "yuml" 	,  255 },
  { "zeta" 	,  950 },
  { "zwj" 	, 8205 },
  { "zwnj" 	, 8204 }
};

#define ENTITIES	(sizeof(m_table) / sizeof(struct entity_conv))

/********************************************************************/

static int entity_cmp(const void *needle,const void *haystack)
{
  const char               *key = needle;
  const struct entity_conv *hay = haystack;
  
  assert(needle   != NULL);
  assert(haystack != NULL);
  
  return strcmp(key,hay->name);
}

/********************************************************************/

char *entity_conversion(const char *s)
{
  FILE   *tmp;
  char   *t  = NULL;
  size_t  sz = 0;
  
  assert(s != NULL);
  
  tmp = open_memstream(&t,&sz);

  while(*s)
  {
    if (*s == '&')
    {
      const struct entity_conv *pe;
      char                      entity[BUFSIZ];
      char                     *p;
      
      if (*++s == '#')
      {
        fputc('&',tmp);
        fputc(*s++,tmp);
        continue;
      }
      
      for ( p = entity ; *s != ';' ; s++,p++)
        *p = *s;
      *p = '\0';
      s++;
      
      pe = bsearch(entity,m_table,ENTITIES,sizeof(struct entity_conv),entity_cmp);
     
      if (pe == NULL)
        fputc('?',tmp);
      else
        fprintf(tmp,"&#%d;",pe->value);
    }
    else
      fputc(*s++,tmp);
  }
  
  fclose(tmp);
  return t;
}

/***********************************************************************/

static bool char_entity(char **tag,size_t *ps,int c)
{
  assert(tag != NULL);
  assert(ps  != NULL);
  
  switch(c)
  {
    case '<':  *tag = "&lt;";   *ps = 4; return true;
    case '>':  *tag = "&gt;";   *ps = 4; return true;
    case '&':  *tag = "&amp;";  *ps = 5; return true;
    case '"':  *tag = "&quot;"; *ps = 6; return true;
    case '\'': *tag = "&apos;"; *ps = 6; return true;
    default:                             return false;
  }
}

/**********************************************************************/

char *entity_encode(const char *s)
{
  FILE   *out;   
  char   *text;
  size_t  size;
  char   *entity;
  size_t  dummy;
 
  assert(s      != NULL);

  out = open_memstream(&text,&size);

  for ( ; *s ; s++)
  {
    if (char_entity(&entity,&dummy,*s))
      fputs(entity,out);
    else
      fputc(*s,out);
  }

  fclose(out);
  return text;
}

/********************************************************************/

FILE *fentity_encode_onread(FILE *in)
{
  return fopencookie(in,"r",(_IO_cookie_io_functions_t)
  				{
  				  fer_read,
  				  fer_write,
  				  fnop_seek,
  				  fnop_close
  				});
}

/*******************************************************************/

FILE *fentity_encode_onwrite(FILE *out)
{
  return fopencookie(out,"w",(_IO_cookie_io_functions_t)
  				{
  				  few_read,
  				  few_write,
  				  fnop_seek,
  				  fnop_close
  				});
}

/******************************************************************/

static __ssize_t fer_read(void *cookie,char *buffer,size_t bytes)
{
  FILE   *realin = cookie;
  size_t  s      = 0;
  char   *replace;
  size_t  repsize;
  int     c;
  
  if (feof(realin))
    return 0;
  
  while(bytes)
  {
    c = fgetc(realin);
    if (c == EOF) return s;
    
    if (char_entity(&replace,&repsize,c))
    {
      if (bytes < repsize)
      {
        ungetc(c,realin);
        return s;
      }
      
      memcpy(buffer,replace,repsize);
      buffer += repsize;
      bytes  -= repsize;
      s      += repsize;
    }
    else
    {
      *buffer++ = c;
      bytes--;
      s++;
    }
  }
  
  return s;
}

/*******************************************************************/

static __ssize_t few_read(void *cookie,char *buffer,size_t bytes)
{
  assert(0);
  return 0;
}

/******************************************************************/

static __ssize_t fer_write(void *cookie,const char *buffer,size_t bytes)
{
  assert(0);
  return 0;
}

/****************************************************************/

static __ssize_t few_write(void *cookie,const char *buffer,size_t bytes)
{
  FILE   *realout = cookie;
  size_t  size    = bytes;
  char   *replace;
  size_t  repsize;

  while(bytes)
  {
    if (char_entity(&replace,&repsize,*buffer))
      fputs(replace,realout);
    else
      fputc(*buffer,realout);
    
    buffer++;
    bytes--;
  }
  
  return size;
}

/*******************************************************************/

static int fnop_seek(void *cookie,_IO_off64_t *pos,int whence)
{
  assert(0);
  errno = ENXIO;
  return -1;
}

/******************************************************************/

static int fnop_close(void *cookie)
{
  return 0;
}

/*******************************************************************/

