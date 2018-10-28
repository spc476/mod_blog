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

local function numposts(year,month)
  local sum = 0
  
  for day = 1 , 31 do
    local fname = string.format("%s/%04d/%02d/%02d/titles",CONF.basedir,year,month,day)
    local f = io.open(fname,"r")
    if f then
      for _ in f:lines() do
        sum = sum + 1
      end
      f:close()
    end
  end
  
  return sum
end

-- ***********************************************************************

for year = g_start.year , g_end.year do
  local first
  local last
  
  if year == g_start.year then
    first = g_start.month
  else
    first = 1
  end
  
  if year == g_end.year then
    last = g_end.month
  else
    last = 12
  end
  
  for month = first , last do
    local sum = numposts(year,month)
    local hdr = os.date("%b %Y",os.time { year = year , month = month , day = 1 })
    print(hdr,sum)
  end
end
