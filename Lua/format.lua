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
local lpeg   = require "lpeg"

local Carg = lpeg.Carg
local Cmt  = lpeg.Cmt
local Cs   = lpeg.Cs
local C    = lpeg.C
local P    = lpeg.P
local R    = lpeg.R
local S    = lpeg.S

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

local tex = P"``"  / ENTITY.ldquo
          + P"''"  / ENTITY.rdquo
          + P"---" / ENTITY.mdash
          + P"--"  / ENTITY.ndash
          + P"..." / "\226\128\166"
          + P".."  / "\226\128\165"
          + P"\\ " / ""
          + P"&amp;"
          + P"&lt;"
          + P"&quot;"
          + P"&apos;"

local entity = P"&#" * C(R"09"^1)             * P";" / utf8.char
             + P"&"  * C(R("az","AZ","09")^1) * P";" / ENTITY

local italic = Cmt(P"//"   * Carg(1),stack "i")
local bold   = Cmt(P"**"   * Carg(1),stack "b")
local em     = Cmt(P"/"    * Carg(1),stack "em")
local strong = Cmt(P"*"    * Carg(1),stack "strong")
local strike = Cmt(P"+"    * Carg(1),stack "del")
local code   = Cmt(P"="    * Carg(1),stack "code")

local scheme   = abnf.ALPHA * (abnf.ALPHA + abnf.DIGIT + S"+-.")^0 * P":"
local toentity = P"&" / "&amp;"
               + P'"' / "&quot;"
               + (P(1) - P"]")
local urltext  = Cs(toentity^1)
local totext   = tex + entity + italic + bold + em + strong + strike + code
               + (P(1) - P"]")
local linktext  = Cs(totext^0)

local link     = P"[[" * urltext * P"][" * linktext * P"]]"
               / function(href,text)
                   local class
                   
                   if scheme:match(href) then
                     class = "external"
                   else
                     class = "local"
                   end
                   
                   return string.format(
                   	[[<a class="%s" href="%s">%s</a>]],
                   	class,
                   	href,
                   	text
                   )
                 end                   
                 
local char = tex +  entity
           + link
           + italic + bold + em + strong + strike + code
           + P(1)

local text = Cs(char^0)

local data = io.stdin:read("*a")
io.stdout:write(text:match(data,1,{ stack = {}}))
