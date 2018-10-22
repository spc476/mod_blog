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
-- *headerline	field2	field3	field4
-- dataitem	data2	data3	data4
-- **
--
-- 	** = repeat header as footer
--
-- caption, header and footer marker are optional.
-- Fields are tab-delimeted.
-- *******************************************************************
-- luacheck: ignore 611

local lpeg = require "lpeg"

local function TDn(c)
  return string.format([[<td class="num">%s</td>]],c)
end

local function TD(c)
  return string.format([[<td>%s</td>]],c)
end

local function TH(c)
  return string.format([[<th>%s</th>]],c)
end

local HEADER = "NOTHING"

local Cc = lpeg.Cc
local C  = lpeg.C
local P  = lpeg.P
local R  = lpeg.R
local S  = lpeg.S

local nl        = P"\n"
local tab       = P"\t"
local text	= R(" ~")^1
local int       = P"0" + (R"19" * R"09"^0)
local frac      = P"." * R"09"^1
local exp       = S"Ee" * S"+-"^-1 * R"09"^1
local number    = (P"-"^-1 * int * frac^-1 * exp^-1) / tonumber
local tr        = C(number) * tab^-1 / function(c) return TDn(c) end
                + C(text)   * tab^-1 / function(c) return TD(c)  end
local row       = (Cc("    <tr>") * tr^1 * Cc("</tr>\n") * nl)^1
                / function(...)
                    return table.concat { ... }
	          end
local th        = C(text) * tab^-1 / function(c) return TH(c) end
local header    = P"*"
                * (
                      Cc("  <thead>\n    <tr>")
                    * th^1
                    * Cc("</tr>\n  </thead>\n")
                    * nl
                  )
                / function(...)
                    local t = { ... }
                    HEADER = table.concat(t,"",2,#t-1)
                    return table.concat(t)
                  end
local footer    = P"**\n"
                * Cc("  <tfoot>\n    <tr>")
                * "" / function(c) return c .. HEADER end
                * Cc("</tr>\n  </tfoot>\n")
local caption   = P"#"
                * (Cc("  <caption>") * C(text) * Cc("</caption>\n") * nl)
                / function(...)
                    return table.concat{ ... }
                  end
local htable    = (
                      Cc("<table>\n")
                    * caption^-1
                    * header^-1
                    * Cc("  <tbody>\n")
                    * row
                    * Cc("  </tbody>\n")
                    * footer^-1
                    * Cc("  </tbody>\n</table>\n")
                  )
                * P";\n"^-1
                / function(...)
                    return table.concat { ... }
                  end

print(htable:match(io.stdin:read("*a")))
