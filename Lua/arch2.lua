#!/usr/bin/env lua
-- ************************************************************************
-- Copyright 2018 by Sean Conner.  All Rights Reserved.
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

local lpeg    = require "lpeg"
local CONF    = setmetatable({},{ __index = _G })
local g_start
local g_end

-- ***********************************************************************

do
  local number     = lpeg.R"09"^1 / tonumber
  local parse_date = lpeg.Ct(
                        lpeg.Cg(number,"year")  * lpeg.P"/" *
                        lpeg.Cg(number,"month") * lpeg.P"/" *
                        lpeg.Cg(number,"day")   * lpeg.P"." *
                        lpeg.Cg(number,"part")
                )
                
  local function getdate(fname)
    local name = CONF.basedir .. "/" .. fname
    local f    = io.open(name,"r")
    local d    = f:read("*a")
    f:close()
    return parse_date:match(d)
  end
  
  local fname = arg[1] or os.getenv "BLOG_CONFIG" or "blog.conf"
  
  if _VERSION == "Lua 5.1" then
    local chunk = loadfile(fname)
    setfenv(chunk,CONF)
    chunk()
  else
    local chunk = loadfile(fname,"t",CONF)
    chunk()
  end
  
  setmetatable(CONF,nil)
  g_start = getdate(".first")
  g_end   = getdate(".last")
end

-- ***********************************************************************

local function anyposts(year,month)
  for day = 1 , 31 do
    local fname = string.format("%s/%04d/%02d/%02d/1",CONF.basedir,year,month,day)
    local f = io.open(fname,"r")
    if f then
      f:close()
      return true
    end
  end
  return false
end

-- ***********************************************************************

io.stdout:write("<table>\n  <tbody\n")

for year = g_end.year , g_start.year , -1 do
  io.stdout:write(string.format("    <tr><th>%d</th>",year))
  for month = 1 , 12 do
    local date = os.time { year = year , month = month , day = 1 }
    if anyposts(year,month) then
      io.stdout:write(os.date([[<td><a class="local" href="/%Y/%m">%b</a></td>]],date))
    else
      io.stdout:write(os.date([[<td>%b</td>]],date))
    end
  end
  io.stdout:write("</tr>\n")
end

io.stdout:write("  </tbody>\n</table>\n")
os.exit(0)
