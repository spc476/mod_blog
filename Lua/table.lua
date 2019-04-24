#!/usr/bin/env lua
-- ************************************************************************
-- Copyright 2005 by Sean Conner.  All Rights Reserved.
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
--
-- ========================================================================
-- Generate a table from text.
--
-- Format:
--
-- #this is a caption
-- *headerline  field2  field3  field4
-- **footerline field2  field3  field4
-- dataitem     data2   data3   data4
--
-- caption, header and footer marker are optional.
-- Fields are tab-delimeted.
-- *******************************************************************
-- luacheck: ignore 611

local lpeg = require "lpeg"

local Cc = lpeg.Cc
local Cs = lpeg.Cs
local P  = lpeg.P
local R  = lpeg.R
local S  = lpeg.S

local nl      = P"\n"
local tab     = P"\t"
local text    = R(" ~","\128\255")^1
local int     = P"0" + (R"19" * R"09"^0)
local frac    = P"." * R"09"^1
local exp     = S"Ee" * S"+-"^-1 * R"09"^1
local number  = (P"-"^-1 * int * frac^-1 * exp^-1)

local td      = Cc'<td class="num">' * number * Cc'</td>' * tab^-1
              + Cc'<td>'             * text   * Cc'</td>' * tab^-1
local th      = Cc'<th>'             * text   * Cc'</th>' * tab^-1
local tr      = Cc'    <tr>' * td^1 * Cc'</tr>' * nl

local caption = P"#" / ""
              * Cc'  <caption>' * text * Cc'</caption>' * nl
local header  = P"*" / ""
              * Cc'  <thead>\n    <tr>' * th^1 * Cc'</tr>\n  </thead>' * nl
local footer  = P"**" / ""
              * Cc'  <tfoot>\n    <tr>' * th^1 * Cc'</tr>\n  </tfoot>' * nl
local htable  = Cs(
                      Cc'<table>\n'
                    * caption^-1
                    * header^-1
                    * footer^-1
                    * Cc'  <tbody>\n' * tr^1 * Cc'  </tbody>\n'
                    * Cc'</table>\n'
                  )
                  
print(htable:match(io.stdin:read("*a")))
