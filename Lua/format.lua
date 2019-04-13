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

local ENTITY = require "org.conman.const.entity"
local abnf   = require "org.conman.parsers.abnf"
local url    = require "org.conman.parsers.url"
local uchar  = require "org.conman.parsers.ascii.char"
             + require "org.conman.parsers.utf8.char"
local lpeg   = require "lpeg"

local Carg = lpeg.Carg
local Cmt  = lpeg.Cmt
local Cc   = lpeg.Cc
local Cs   = lpeg.Cs
local C    = lpeg.C
local P    = lpeg.P
local R    = lpeg.R
local S    = lpeg.S

-- ********************************************************************

local function url_class(href)
  local u = url:match(href)
  
  if u.scheme then
    if u.host:match "conman.org" then
      return "site"
    else
      return "external"
    end
  else
    return "local"
  end
end

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

        -- ------------------
        -- simple substutions
        -- ------------------
        
local tex = P"``"    / ENTITY.ldquo
          + P"''"    / ENTITY.rdquo
          + P"-----" / "<hr>"
          + P"---"   / ENTITY.mdash
          + P"--"    / ENTITY.ndash
          + P"..."   / "\226\128\166"
          + P".."    / "\226\128\165"
          + P"<-"    / "\226\134\144"
          + P"->"    / "\226\134\146"
          + P"\\ "   / ""
          + P"&amp;"
          + P"&lt;"
          + P"&quot;"
          + P"&apos;"
          
        -- -----------------------------------------
        -- convert HTML entities to UTF-8 characters
        -- -----------------------------------------
        
local entity = P"&#" * C(R"09"^1)             * P";" / utf8.char
             + P"&"  * C(R("az","AZ","09")^1) * P";" / ENTITY
             
        -- --------------------------------------------------------------
        -- Redact data.  Format: {{redacted data}}
        --
        -- This will convert text between the {{ and }} with X's, with a
        -- soft-hypen every 5 characters.  This text will be placed in
        -- a <SPAN> with a CLASS attribute of 'cut'.
        -- --------------------------------------------------------------
        
local cut_c    = (uchar - P"}}") / "X"
local cut_char = cut_c * cut_c^-4 * (#cut_c * Cc"\194\173")^-1
local cut      = (P"{{" / '<span class="cut">')
               * Cs(cut_char^0)
               * (P"}}" / '</span>')
               
        -- ---------------------------------------------------------
        -- Various shorthand notations for common HTML styling tags
        -- ---------------------------------------------------------
        
local italic = Cmt(P"//" * Carg(1),stack "i")
local bold   = Cmt(P"**" * Carg(1),stack "b")
local em     = Cmt(P"/"  * Carg(1),stack "em")
local strong = Cmt(P"*"  * Carg(1),stack "strong")
local strike = Cmt(P"+"  * Carg(1),stack "del")
local code   = Cmt(P"="  * Carg(1),stack "code")

        -- ----------------------------------
        -- Handle paragraphs automagically
        -- ----------------------------------
        
local paras = C"\n\n" * C(uchar - S"#-*") / "%1<p>%2"
local parae = C(uchar) * #(P'\n\n' + P'\n' * P(-1) + P(-1)) / "%1</p>"
local para  = paras + parae

        -- ------------------------------------------------------------
        -- HTML links.  Format: [[href][linktext]]
        --
        -- NOTE:  relative links get a CLASS attribute of 'local',
        --        URIs to 'conman.org' get a CLASS attribute of 'site'
        --        All others get a CLASS attribute of 'external'
        -- ------------------------------------------------------------
        
local urltext  = C((P(1) - P']')^1)
local totext   = tex + entity + italic + bold + em + strong + strike + code
               + (P(1) - P"]")
local linktext = Cs(totext^0)
local link     = P"[[" * urltext * P"][" * linktext * P"]]"
               / function(href,text)
                   return string.format(
                        [[<a class="%s" href="%s">%s</a>]],
                        url_class(href),
                        href,
                        text
                   )
                 end
                 
        -- ------------------
        -- Handle HTML tags
        -- ------------------
        
local tag       = abnf.ALPHA * (abnf.ALPHA + abnf.DIGIT)^0
local aname     = abnf.ALPHA * (abnf.ALPHA + P"-")^0
local SPACE     = S" \t\r\n"
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
local htmltag = P"<"  * tag * attrs^0 * P">"
              + P"</" * tag * P">"
              
        -- -----------------------------
        -- Shorthand for <H1> .. <H5>
        -- -----------------------------
        
local hchar  = tex + entity + uchar
local header = C"\n***** " * Cs(hchar^1) * #P"\n" / "\n<h5>%2</h5>"
             + C"\n**** "  * Cs(hchar^1) * #P"\n" / "\n<h4>%2</h4>"
             + C"\n*** "   * Cs(hchar^1) * #P"\n" / "\n<h3>%2</h3>"
             + C"\n** "    * Cs(hchar^1) * #P"\n" / "\n<h2>%2</h2>"
             + C"\n* "     * Cs(hchar^1) * #P"\n" / "\n<h1>%2</h1>"
             
-- ********************************************************************
-- #+BEGIN blocks.  We start with #+BEGIN_SRC
--
--      <sol> = start-of-line
--      <eol> = end-of-line
--      <sp>  = whitespace
--
--      <sol> '#+BEGIN_SRC' <space> <language> <eol>
--      <raw text>
--      <sol> '#+END_SRC' <eol>
--
-- ********************************************************************

local src_char  = P"<" / "&lt;"
                + P"&" / "&amp;"
                + P(1) - P"\n#+END_SRC\n"
                
local begin_src = C(S" \t"^0) * C(R("AZ","az")^1) * C(P"\n")
                / '\n<pre class="language-%2" title="%2">\n'
                * Cs(src_char^0)
                * (P"\n#+END_SRC" * #P"\n" / "\n</pre>\n")
                
-- ********************************************************************
-- #+BEGIN_TABLE
--
--      <sol> '#+BEGIN_TABLE' <eol>
--      [ <sol> '*'  <text> 0(<ht> <text>) <eol> ] -- header row
--      [ <sol> '**' <text> 0(<ht> <text>) <eol> ] -- footer row
--      1(<sol> <text>      0(<ht> <text>) <eol> ) -- data row
--      <sol> '#+END_TABLE' <eol>
--
-- ********************************************************************

local int    = P"0" + (R"19" * R"09"^0)
local frac   = P"." * R"09"^1
local exp    = S"Ee" * S"+-"^-1 * R"09"^1
local number = (P"-"^-1 * int * frac^-1 * exp^-1)

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
local td            = Cc'<td class="num">' * number      * Cc'</th>' * (P"\t" / " ")^-1
                    + Cc'<th>'             * Cs(hchar^1) * Cc'</th>' * (P"\t" / " ")^-1
local tr            = -P"#+END_TABLE" * Cc'    <tr>' * td^1 * Cc'</td>' * P"\n"
local table_body    = Cc'  <tbody>\n'
                    * tr^1
                    * Cc'  </tbody>\n'
local begin_table   = Cc"\n<table>\n" * table_caption
                    * table_header^-1
                    * table_footer^-1
                    * table_body
                    * (P"#+END_TABLE" * #P"\n" / "</table>\n")
                    
-- ********************************************************************
-- #+BEGIN_EMAIL
--
--      <sol> '#+BEGIN_EMAIL' [ <sp> 'all' <sp>0 ] <eol>
--      <email headers>
--      <eol>
--      <email body>
--      <sol> '#+END_EMAIL' <eol>
--
-- ********************************************************************

local LWSP        = (abnf.WSP + (abnf.CRLF * abnf.WSP) / " ")
local email_opt   = S" \t"^0 * P"all" * S" \t"^0
                  * Cmt(Carg(1),function(_,pos,s) s.email_all = true return pos end)
local hdr_name    = (P(1) - P":")^1
local hdr_char    = P"<" / "&lt;"
                  + P"&" / "&amp;"
                  + P">" / "&gt;"
local hdr_text    = LWSP + hdr_char + cut + tex + entity + uchar
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
local email_text  = header + htmltag + tex + entity + link
                  + cut + italic + bold + em + strong + strike + code
                  + para
                  + (P(1) - P"#+END_EMAIL")
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
                  * (P"#+END_EMAIL" * #P"\n" / "\n</blockquote>")
                  
-- ********************************************************************
-- #+BEGIN_QUOTE
--
--      <sol> #+BEGIN_QUOTE
--      <text>
--      <sol> #+END_QUOTE
--
-- ********************************************************************

local quote_text  = header + htmltag + tex + entity + link
                  + cut + italic + bold + em + strong + strike + code
                  + para
                  + (P(1) - P"#+END_QUOTE")
                  
local begin_quote = Cmt(
                      Carg(1),
                      function(_,pos,state)
                        local htag = '\n<blockquote'
                        if state.quote.cite then
                          htag = htag .. string.format(" cite=%q",state.quote.cite)
                        end
                        if state.quote.title then
                          htag = htag .. string.format(" title=%q",state.quote.title)
                        end
                        htag = htag .. ">"
                        return pos,htag
                      end
                    )
                  * quote_text^0
                  * Cmt(
                      P"#+END_QUOTE" * #P"\n" * Carg(1),
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
-- #+BEGIN_COMMENT
--
--      <sol> #+BEGIN_COMMENT <eol>
--      text
--      <sol> #+END_COMMENT <eol>
--
-- NOTE:        These blocks cannot nest.  Bad things will happen.
-- ********************************************************************

local begin_comment = (P(1) - P"\n#+END_COMMENT")^0 / "<!-- comment -->"
                    * (P"\n#+END_COMMENT" * #P"\n") / ""
                    
-- ********************************************************************
-- #+ATTR_QUOTE
--
--      <sol> '#+ATTR_QUOTE:' <sp> [<cite> / <title> / <via-url> | <via-title> ] <eol>
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
                  
local attr_quote = S" \t"^1 / ""
                 * Cmt(Carg(1) * quote_attrs * C(S" \t"^1) * C((P(1) - P"\n")^1),
                   function(_,pos,state,cite,_,text)
                     state.quote[cite] = text
                     return pos,""
                   end)
                   
-- ********************************************************************
-- Top level #+BEGIN blocks definition
-- ********************************************************************

local begin  = P"_SRC"      / "" * begin_src
             + P"_TABLE"    / "" * begin_table
             + P"_EMAIL"    / "" * begin_email
             + P"_QUOTE"    / "" * begin_quote
             + P"_COMMENT"  / "" * begin_comment
             
local battr  = P"_QUOTE:"   / "" * attr_quote

local blocks = P"\n#+BEGIN" / "" * begin
             + P"\n#+ATTR"  / "" * battr
             
-- ********************************************************************

local char = blocks
           + header
           + htmltag
           + tex +  entity
           + link
           + cut + italic + bold + em + strong + strike + code
           + para
           + P(1)
           
local text = Cs(char^0)

-- ********************************************************************

local state =
{
  stack     = {},
  email_all = false,
  quote     = {}
}

local data = io.stdin:read("*a")
io.stdout:write(text:match(data,1,state))