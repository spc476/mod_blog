#!/usr/bin/env lua
-- ************************************************************************
-- Copyright 2019 by Sean Conner.  All Rights Reserved.
--
-- This program is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License
-- as published by the Free Software Foundation; either version 2
-- of the License, or (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
--
-- Comments, questions and criticisms can be sent to: sean@conman.org
--
-- ************************************************************************
-- luacheck: ignore 611

local ENTITY  = require "org.conman.const.entity"
local abnf    = require "org.conman.parsers.abnf"
local url     = require "org.conman.parsers.url"
local uchar   = require "org.conman.parsers.ascii.char"
              + require "org.conman.parsers.utf8.char"
local allchar = require "org.conman.parsers.ascii"
              + require "org.conman.parsers.utf8.char"
local lpeg    = require "lpeg"

local Carg = lpeg.Carg
local Cmt  = lpeg.Cmt
local Cc   = lpeg.Cc
local Cs   = lpeg.Cs
local B    = lpeg.B
local C    = lpeg.C
local P    = lpeg.P
local R    = lpeg.R
local S    = lpeg.S

-- ********************************************************************
-- usage:       for chunk,data in jpeg_sections(file) do ... end
-- desc:        Iterator to cycle through chunks in a JPEG file
-- input:       file (userdata/FILE*) already open JPEG file
-- return:      chunk (integer) JPEG chunk type
--              data (bin) raw data for that chunk
--
-- Note:        Not all chunks are decoded propery---this is enough to
--              get the image size though, which is all I wanted.
-- ********************************************************************

local function jpeg_sections(file)
  file:seek('set',2)
  return function()
    local chunk = string.unpack(">I2",file:read(2) or "\255\217")
    if chunk == 0xFFD9 then return nil end
    
    local size = string.unpack(">I2",file:read(2))
    if size == 0 then
      return chunk,""
    else
      return chunk,file:read(size - 2)
    end
  end
end

-- ********************************************************************
-- usage:       width,height = image_size(filename)
-- desc:        Return the image size in pixels
-- input:       filename (string) filename of image
-- return:      width (integer) width in pixels
--              height (integer) height in pixels
-- ********************************************************************

local function image_size(filename)
  local f      = io.open(filename,"rb")
  local header = f:read(24)
  local width
  local height
  
  if header:sub(1,3) == "GIF" then
    width,height = string.unpack("<I2I2",header,7)
  elseif header:sub(1,8) == "\137PNG\r\n\26\n" then
    width,height = string.unpack(">I4I4",header,17)
  elseif header:sub(1,2) == "\255\216" then
    for chunk,data in jpeg_sections(f) do
      if chunk == 0xFFC0 then
        height,width = string.unpack(">I2I2",data,2)
        break
      end
    end
  else
    width  = 0
    height = 0
  end
  
  f:close()
  return width,height
end

-- ********************************************************************
-- usage:       mime = image_type(filename)
-- desc:        Return the image type
-- input:       filename (string) filename of image
-- return:      mime (string) MIME type of image
-- ********************************************************************

local function image_type(filename)
  local f      = io.open(filename,"r")
  local header = f:read(8)
  local mime
  
  if header:sub(1,3) == "GIF" then
    mime = "image/gif"
  elseif header:sub(1,8) == "\137PNG\r\n\26\n" then
    mime = "image/png"
  elseif header:sub(1,2) == "\255\216" then
    mime = "image/jpeg"
  else
    mime = "text/plain"
  end
  
  f:close()
  return mime
end

-- ********************************************************************
-- usage:       class = url_class(href)
-- desc:        Return a class attribute for a link
-- input:       href (string) a URL
-- return:      class (string) a class attribute
-- ********************************************************************

local function url_class(href)
  local u = url:match(href)
  
  if u.scheme then
    if not u.host then
      return "external"
    elseif u.host:match "conman.org" then
      return "site"
    else
      return "external"
    end
  else
    return "local"
  end
end

-- ********************************************************************
-- usage:       f = stack(tag)
-- desc:        Return a function for lpeg.Cmt() to track generated
--              HTML tags
-- input:       tag (string) HTML tag name
-- return:      f (function) a function suitable for lpeg.Cmt() to
--                      | generate an opening or closing tag.
-- ********************************************************************

local function stack(tag)
  local st = "<"  .. tag .. ">"
  local et = "</" .. tag .. ">"
  
  return function(_,position,state)
    if state.stack[#state.stack] == tag then
      table.remove(state.stack)
      return position,et
    else
      table.insert(state.stack,tag)
      return position,st
    end
  end
end

-- ********************************************************************

        -- ----------------------------------------
        -- acronym expansion into
        --    <abbr title="expansion">ABBR</abbr>
        -- ----------------------------------------
        
local abbrex = C("IPv4")
             + C("IPv6")
             + C("GHz")
             + C("MHz")
             + C("POP3")
             + C("D&amp;D5")
             + C("D&amp;D")
             + C("K&amp;R")
             + C("AT&amp;T")
             + C("NaNoWriMo")
             + C("NaNoGenMo")
             + C("I/O")
             + P"D&D5" / "D&amp;D5"
             + P"D&D"  / "D&amp;D"
             + P"K&R"  / "K&amp;R"
             + P"AT&T" / "AT&amp;T"
             + C("mDNS")
             + C(R"AZ" * ((R"ax" * #R"AZ") + R"AZ")^1)
local abbr   = Cmt(
                  abbrex * Carg(1),
                  function(_,pos,acronym,state)
                    if state.abbr[acronym] then
                      return pos,string.format('<abbr title="%s">%s</abbr>',
                        state.abbr[acronym],
                        acronym
                      )
                    else
                      return pos
                    end
                  end
                )
                
        -- ------------------
        -- simple substutions
        -- ------------------
        
local function fraction(text,name)
  return B(P(1) - R"09") * P(text) * #(P(1) - R"09")
       / ENTITY[name]
end

local tex = P"``"    / ENTITY.ldquo
          + P"''"    / ENTITY.rdquo
          + P"-----" / "<hr>"
          + P"---"   / ENTITY.mdash
          + P"--"    / ENTITY.ndash
          + P"(TM)"  / ENTITY.trade
          + P"(C)"   / ENTITY.copy
          + P"(R)"   / ENTITY.reg
          + P"..."   / ENTITY.hellip
          + P".."    / ENTITY.nldr
          + P"<-"    / ENTITY.LeftArrow
          + P"->"    / ENTITY.RightArrow
          + P"^st"   / "<sup>st</sup>"
          + P"^nd"   / "<sup>nd</sup>"
          + P"^rd"   / "<sup>rd</sup>"
          + P"^th"   / "<sup>th</sup>"
          + P"\\-"   / ENTITY.shy
          + P"\\_"   / ENTITY.nbsp
          + P"\\ "   / ""
          + P" /\n"  / "<br>\n"
          + P"\\&"   / "&amp;"
          + P"\\<"   / "&lt;"
          + P"\\/"   / "/"
          + fraction("1/4",'frac12')
          + fraction("1/2",'frac12')
          + fraction("3/4",'frac34')
          + fraction("1/3",'frac13')
          + fraction("2/3",'frac23')
          + fraction("1/5",'frac15')
          + fraction("2/5",'frac25')
          + fraction("3/5",'frac35')
          + fraction("4/5",'frac45')
          + fraction("1/6",'frac16')
          + fraction("5/6",'frac56')
          + fraction("1/8",'frac18')
          + fraction("3/8",'frac38')
          + fraction("5/8",'frac58')
          + fraction("7/8",'frac78')
          + C"\\" * C(uchar) / "%2"
          
        -- -----------------------------------------
        -- convert HTML entities to UTF-8 characters
        -- -----------------------------------------
        
local entity = P"&amp;"
             + P"&lt;"
             + P"&quot;"
             + P"&apos;"
             + P"&#" * C(R"09"^1)             * P";" / utf8.char
             + P"&"  * C(R("az","AZ","09")^1) * P";" / ENTITY
             
        -- --------------------------------------------------------------
        -- Redact data.  Format: {{redacted data}}
        --
        -- This will convert text between the {{ and }} with X's, with a
        -- soft-hypen every 5 characters.  This text will be placed in
        -- a <SPAN> with a CLASS attribute of 'cut'.
        -- --------------------------------------------------------------
        
local cut_c    = (allchar - P"}}") / "X"
local cut_char = cut_c * cut_c^-4 * (#cut_c * Cc"\194\173")^-1
local cut      = (P"{{" / '<span class="cut">')
               * Cs(cut_char^0)
               * (P"}}" / '</span>')
               
        -- ---------------------------------------------------------
        -- Various shorthand notations for common HTML styling tags
        -- ---------------------------------------------------------
        
local style = Cmt(P"**" * Carg(1),stack "strong")
            + Cmt(P"*"  * Carg(1),stack "em")
            + Cmt(P"`"  * Carg(1),stack "code")
            + Cmt(P"++" * Carg(1),stack "del")
            
        -- ----------------------------------
        -- Handle paragraphs automagically
        -- ----------------------------------
        
local paras = C"\n\n"  * #(uchar - S"<#-*") / "%1<p>"
local parae = C(uchar) * #(P'\n\n' + P'\n' * P(-1) + P(-1)) / "%1</p>"
local para  = paras + parae

        -- ------------------------------------------------------------
        -- HTML links.  Formats:
        --              {^href linktext}
        --              {^href}
        --
        -- NOTE:  relative links get a CLASS attribute of 'local',
        --        URIs to 'conman.org' get a CLASS attribute of 'site'
        --        All others get a CLASS attribute of 'external'
        -- ------------------------------------------------------------
        
local SPACE    = S" \t\r\n"
local urlchar  = P"&" / "&amp;"
               + R"!~" - P'}'
local urltext  = Cs(urlchar^1)
local totext   = abbr + tex + entity + style
               + (abnf.HTAB + abnf.CRLF) / " "
               + (uchar - P"}")
local linktext = Cs(totext^1)
local link     = P"{^" * urltext * SPACE^1 * linktext * P"}"
               / function(href,text)
                   return string.format(
                        [[<a class="%s" href="%s">%s</a>]],
                        url_class(href),
                        href,
                        text
                   )
                 end
               + P"{^" * urltext * P"}"
               / function(href)
                   return string.format(
                     [[<code><a class="%s" href="%s">%s</a></code>]],
                     url_class(href),
                     href,
                     href
                   )
                 end
                 
        -- ------------------
        -- Handle HTML tags
        -- ------------------
        
local tag       = abnf.ALPHA * (abnf.ALPHA + abnf.DIGIT)^0
local aname     = abnf.ALPHA * (abnf.ALPHA + P"-")^0
local EQ        = SPACE^0 * P"=" * SPACE^0
local htmltext  = tex + entity + (-SPACE * uchar)
local htmlchard = tex + entity + (-P'"'  * uchar)
local htmlchars = tex + entity + (-P"'"  * uchar)
local htmltrans = P'"' * Cs(htmlchard^0) * P'"'
                + P"'" * Cs(htmlchars^0) * P"'"
                +        Cs(htmltext^1)
local value     = P'"' * htmlchard^0 * P'"'
                + P"'" * htmlchars^0 * P"'"
                +        htmltext^1
local attr      = P"title" *  EQ * htmltrans
                + P"alt"   *  EQ * htmltrans
                + aname    * (EQ * value)^-1
local attrs     = SPACE^0 * attr
local htmltag   = P"<"    * tag * attrs^0 * SPACE^0 * P">"
                + P"<!--" * (P(1) - P"-->")^0 * P"-->"
                + P"</"   * tag * P">"
                
        -- -----------------------------
        -- Shorthand for <H1> .. <H5>
        -- -----------------------------
        
local hchar  = cut + abbr + tex + entity + uchar
local header = C"\n***** " * Cs(hchar^1) * #P"\n" / "\n<h5>%2</h5>"
             + C"\n**** "  * Cs(hchar^1) * #P"\n" / "\n<h4>%2</h4>"
             + C"\n*** "   * Cs(hchar^1) * #P"\n" / "\n<h3>%2</h3>"
             + C"\n** "    * Cs(hchar^1) * #P"\n" / "\n<h2>%2</h2>"
             + C"\n* "     * Cs(hchar^1) * #P"\n" / "\n<h1>%2</h1>"
             
-- ********************************************************************
-- #+BEGIN blocks.  We start with #+source
--
--      <sol> = start-of-line
--      <eol> = end-of-line
--      <sp>  = whitespace
--
--      <sol> '#+source' <space> <language> <eol>
--      <raw text>
--      <sol> '#-source' <eol>
--
-- ********************************************************************

local src_char  = P"<" / "&lt;"
                + P"&" / "&amp;"
                + allchar - P"\n#-source\n"
local begin_src = C(S" \t"^0) * C(R("AZ","az")^1) * C(P"\n")
                / '\n<pre class="language-%2" title="%2">\n'
                * Cs(src_char^0)
                * (P"\n#-source" * #P"\n" / "\n</pre>\n")
                
-- ********************************************************************
-- #+table
--
--      <sol> '#+table' <eol>
--      [ <sol> '*'  <text> 0(<ht> <text>) <eol> ] -- header row
--      [ <sol> '**' <text> 0(<ht> <text>) <eol> ] -- footer row
--      1(<sol> <text>      0(<ht> <text>) <eol> ) -- data row
--      <sol> '#-table' <eol>
--
-- Note:	If <text> starts with '\f' then it represents a header
--		field within a data row.
-- ********************************************************************

local int           = P"0" + (R"19" * R"09"^0)
local frac          = P"." * R"09"^1
local exp           = S"Ee" * S"+-"^-1 * R"09"^1
local number        = (P"-"^-1 * int * frac^-1 * exp^-1) * #S"\t\n"
local table_caption = C(S" \t"^0) * Cs(hchar^1) * C"\n"
                    / "  <caption>%2</caption>\n"
                    + P"\n" / ""
local th            = Cc'<th>' * Cs(hchar^1) * Cc'</th>' * (P"\t" / " ")^-1
local table_header  = P"*" / ""
                    * Cc'  <thead>\n    <tr>'
                    * th^1
                    * Cc'</tr>\n  </thead>'
                    * P"\n"
local table_footer  = P"**" / ""
                    * Cc'  <tfoot>\n    <tr>'
                    * th^1
                    * Cc'</tr>\n  </tfoot>'
                    * P'\n'
local td            = P"\\f" * C(number)   / '<th class="num">%1</th>' * (P"\t" / " ")^-1
                    + P"\\f" * Cs(hchar^1) / '<th>%1</th>'             * (P"\t" / " ")^-1
                    + C(number)            / '<td class="num">%1</td>' * (P"\t" / " ")^-1
                    + Cs(hchar^1)          / '<td>%1</td>'             * (P"\t" / " ")^-1
local tr            = -P"#-table" * Cc'    <tr>' * td^1 * Cc'</tr>' * P"\n"
local table_body    = Cc'  <tbody>\n'
                    * tr^1
                    * Cc'  </tbody>\n'
local begin_table   = Cc"\n<table>\n" * table_caption
                    * table_header^-1
                    * table_footer^-1
                    * table_body
                    * (P"#-table" * #P"\n" / "</table>\n")
                    
-- ********************************************************************
-- #+email
--
--      <sol> '#+email' [ <sp> 'all' <sp>0 ] <eol>
--      <email headers>
--      <eol>
--      <email body>
--      <sol> '#-email' <eol>
--
-- ********************************************************************

local LWSP        = (abnf.WSP + (abnf.CRLF * abnf.WSP) / " ")
local email_opt   = S" \t"^0 * P"all" * S" \t"^0 / ""
                  * Cmt(Carg(1),function(_,pos,s) s.email_all = true return pos end)
local hdr_name    = (R("AZ","az","--","__","09") - P":")^1
local hdr_char    = P"<" / "&lt;"
                  + P"&" / "&amp;"
                  + P">" / "&gt;"
local hdr_text    = LWSP + hdr_char + abbr + cut + tex + entity + uchar
local hdr_value   = Cc'<dd>' * Cs(hdr_text^1) * Cc'</dd>'
local hdr_generic = Cmt(
                      Carg(1) * C(hdr_name) * P":" * Cs(hdr_text^1) * abnf.CRLF,
                      function(_,pos,s,name,val)
                        if s.email_all then
                          return pos,string.format("  <dt>%s</dt><dd>%s</dd>\n",name,val)
                        else
                          return pos,""
                        end
                      end
                    )
local email_text  = header + htmltag + abbr + tex + entity + link
                  + cut + style
                  + para
                  + (allchar - P"#-email")
local email_hdrs  = (P"From:"    / "    <dt>From</dt>")    * hdr_value * abnf.CRLF
                  + (P"To:"      / "    <dt>To</dt>")      * hdr_value * abnf.CRLF
                  + (P"Subject:" / "    <dt>Subject</dt>") * hdr_value * abnf.CRLF
                  + (P"Date:"    / "    <dt>Date</dt>")    * hdr_value * abnf.CRLF
                  + hdr_generic
local begin_email = Cc'\n<blockquote>\n  <dl class="header">' * email_opt^-1 * P"\n"
                  * email_hdrs^1
                  * Cc'  </dl>\n'
                  * abnf.CRLF * Cc'<p>'
                  * email_text^0
                  * (P"#-email" * #P"\n" / "\n</blockquote>")
                  
-- ********************************************************************
-- #+quote
--
--      <sol> #+quote
--      <text>
--      <sol> #-quote
--
-- ********************************************************************

local title_char  = P"&" / "&amp;"
                  + P"<" / "&lt;"
                  + P">" / "&gt;"
                  + P'"' / "&quot;"
                  + allchar
local title_text  = Cs(title_char^1)
local quote_text  = header + htmltag + abbr + tex + entity + link
                  + cut + style
                  + para
                  + (allchar - P"#-quote")
local begin_quote = Cmt(
                      Carg(1),
                      function(_,pos,state)
                        local htag = '\n<blockquote'
                        if state.quote.cite then
                          htag = htag .. string.format(" cite=%q",title_text:match(state.quote.cite))
                        end
                        if state.quote.title then
                          htag = htag .. string.format(" title=%q",title_text:match(state.quote.title))
                        end
                        htag = htag .. ">"
                        return pos,htag
                      end
                    )
                  * quote_text^0
                  * Cmt(
                      P"#-quote" * #P"\n" * Carg(1),
                      function(_,pos,state)
                        local via
                        local cite
                        local par
                        
                        if state.quote.via_url and state.quote.via_title then
                          via = string.format(
                              [[Via <a class="%s" href="%s">%s</a>, ]],
                              url_class(state.quote.via_url),
                              state.quote.via_url,
                              state.quote.via_title
                          )
                        elseif state.quote.via_url and not state.quote.via_title then
                          via = string.format(
                              [[Via <code><a class="%s" href="%s">%s</a></code>, ]],
                              url_class(state.quote.via_url),
                              state.quote.via_url,
                              state.quote.via_url
                          )
                        elseif not state.quote.via_url and state.quote.via_title then
                          via = string.format([[Via %s, ]],state.quote.via_title)
                        end
                        
                        if state.quote.cite and state.quote.title then
                          cite = string.format(
                              [[<cite><a class="%s" href="%s">%s</a></cite>]],
                              url_class(state.quote.cite),
                              state.quote.cite,
                              state.quote.title
                          )
                        elseif state.quote.cite and not state.quote.title then
                          cite = string.format(
                              [[<cite><code><a class="%s" href="%s">%s</a></code></cite>]],
                              url_class(state.quote.cite),
                              state.quote.cite,
                              state.quote.cite
                          )
                        elseif not state.quote.cite and state.quote.title then
                          cite = string.format([[<cite>%s</cite>]],state.quote.title)
                        end
                        
                        if via or cite then
                          par = string.format(
                              '<p class="cite">%s%s</p>',
                              via or "",
                              cite or ""
                          )
                        end
                        
                        state.quote.cite      = nil
                        state.quote.title     = nil
                        state.quote.via_url   = nil
                        state.quote.via_title = nil
                        
                        return pos,string.format("%s\n</blockquote>\n",par or "")
                      end
                    )
                    
-- ********************************************************************
-- #+photo
--
--      <sol> #+photo <eol>
--      <sol> displayfile [<sp> linkfile] <eol>
--      <sol> <sp> alt=<text> <eol>
--      <sol> <sp> title=<text> <eol>
--      <sol> #-photo <eol>
--
-- NOTE:        If linkfile is not specified, then no link is generated.
--              There can be more than one display file.
-- ********************************************************************

local pf_char  = tex
               + entity
               + P'"' / '&quot;'
               + P"&" / '&amp;'
               + P"<" / '&lt;'
               + uchar
               
local pf_image = P"\n" * #-P"#-photo"
               * C(uchar^1) * ((S" \t^1" * C(uchar^1)) + Cc(nil))
               * P"\n" * S" \t"^1 * P"alt="   * Cs(pf_char^1)
               * P"\n" * S" \t"^1 * P"title=" * Cs(pf_char^1)
               / function(display,target,alt,title)
                   if not target then
                     local width,height = image_size(display)
                     return string.format(
                        '  <img src="%s" width="%d" height="%d" alt="[%s] %s" title="%s">\n',
                        display,
                        width,
                        height,
                        alt,title,
                        title
                      )
                   else
                     local mime         = image_type(target)
                     local width,height = image_size(target)
                     return string.format(
                        '  <a type="%s" class="notype" href="%s"><img src="%s" width="%d" height="%d" alt="[%s] %s" title="%s"></a>\n', -- luacheck: ignore
                        mime,
                        display,
                        target,
                        width,
                        height,
                        alt,title,
                        title
                     )
                   end
                 end
                 
local begin_pf = #P"\n" * Cc'\n<div class="pf">\n'
               * pf_image^1
               * (P"\n#-photo" * #P"\n" / "</div>")
               
-- ********************************************************************
-- #+ATTR_QUOTE
--
--      <sol> '#+ATTR_QUOTE:' <sp> [<cite> | <title> | <via-url> | <via-title> ] <eol>
--
--      <cite>      -> ':cite'      <sp> <url>
--      <title>     -> ':title'     <sp> <text>
--      <via-url>   -> ':via-url'   <sp> <url>
--      <via-title> -> ':via-title' <sp> <text>
--
-- NOTE:        Only one attribute per #+ATTR_QUOTE is supported.
--              Multiple #+ATTR_QUOTE lines can be specified
-- ********************************************************************

local quote_attrs = P":cite"      / "cite"
                  + P":title"     / "title"
                  + P":via-url"   / "via_url"
                  + P":via-title" / "via_title"
local attr_quote  = S" \t"^1 / ""
                  * Cmt(Carg(1) * quote_attrs * C(S" \t"^1) * C((allchar - P"\n")^1),
                    function(_,pos,state,cite,_,text)
                      state.quote[cite] = text
                      return pos,""
                    end)
                    
-- ********************************************************************
-- Top level #+BEGIN blocks definition
-- ********************************************************************

local begin  = P"source"   / "" * begin_src
             + P"table"    / "" * begin_table
             + P"email"    / "" * begin_email
             + P"quote"    / "" * begin_quote
             + P"photo"    / "" * begin_pf
local battr  = P"_QUOTE:"  / "" * attr_quote
local blocks = P"\n#+"     / "" * begin
             + P"\n#+ATTR" / "" * battr
             
-- ********************************************************************
-- Entry header
-- ********************************************************************

local entry_header do
  local sol      = P"\n"^-1
  local echar    = tex + entity + uchar
  local acronyms = (P"\n"^-1 * S" \t"^1) / ""
                 * Cmt(
                          abbrex
                        * C(S" \t"^1)
                        * C(tex + entity + uchar^1)
                        * Carg(1),
                        function(_,pos,abrev,_,expansion,state)
                          state.abbr[abrev] = expansion
                          return pos,""
                        end)
  local abbrh    = (sol * P"abbr:") / "" * acronyms^1
  local author   = sol * P"author:" * Cs(uchar^0)
  local title    = sol * P"title:"  * Cs(echar^0)
  local class    = sol * P"class:"  * Cs(echar^0)
  local status   = sol * P"status:" * Cs(echar^0)
  local date     = sol * P"date:"   * uchar^0
  local adtag    = sol * P"adtag:"  * Cs(echar^0)
  local email    = sol * P"email:"  * uchar^0
  local filter   = sol * P"filter:" * uchar^0
  entry_header   = (
                       author
                     + title
                     + class
                     + status
                     + date
                     + adtag
                     + email
                     + filter
                     + abbrh
                   )^0
end

-- ********************************************************************

local char = blocks
           + header
           + htmltag
           + abbr + tex +  entity
           + link
           + cut + style
           + para
           + allchar
local text = Cs(entry_header * char^0)

-- ********************************************************************

local state =
{
  email_all = false,
  stack     = {},
  quote     = {},
  abbr      = {},
}

local data = io.stdin:read("*a")
io.stdout:write(text:match(data,1,state))
