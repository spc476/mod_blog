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

local lpeg         = require "lpeg"
local CONF         = setmetatable({},{ __index = _G })
local g_categories = {}
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

local parse_tags do
  local char = (lpeg.R" \255" - lpeg.P",")
  local text = lpeg.S" \t"^0 * lpeg.C(char^0)
  parse_tags = lpeg.Ct(text * (lpeg.P"," * text)^0)
end

local function collect_class(when)
  local tdate     = string.format("%04d/%02d/%02d",when.year,when.month,when.day)
  local sclass    = string.format("%s/%s/class", CONF.basedir,tdate)
  local stitles   = string.format("%s/%s/titles",CONF.basedir,tdate)
  local fclass    = io.open(sclass,"r")
  local ftitles   = io.open(stitles,"r")
  local count     = 1
  
  if fclass == nil then return end
  if not ftitles then
    print(stitles)
    os.exit(1)
  end
  
  for class in fclass:lines() do
    local title = ftitles:read("*l")
    if not title then title = "[no title]" end
    local tags = parse_tags:match(class)
    for _,tag in ipairs(tags) do
      if not g_categories[tag] then
        g_categories[tag] = {}
      end
      table.insert(g_categories[tag],string.format("%s.%d\t%s",tdate,count,title))
    end
    count = count + 1
  end
  
  ftitles:close()
  fclass:close()
end

-- *********************************************************************

local function daysinmonth(year,month)
  local days = { 31 , 0 , 31 , 30 , 31 , 30 , 31 , 31 , 30 , 31 , 30 , 31 }
  
  if month == 2 then
    if year % 400 == 0 then return 29 end
    if year % 100 == 0 then return 28 end
    if year %   4 == 0 then return 29 end
    return 28
  end
  
  return days[month]
end

-- *********************************************************************

local function addday(today)
  today.day = today.day + 1
  if today.day > daysinmonth(today.year,today.month) then
    today.day   = 1
    today.month = today.month + 1
    if today.month == 13 then
      today.month = 1
      today.year  = today.year + 1
    end
  end
  today.hour = 1
end

-- ********************************************************************

local function timecmp(a,b)
  local res
  
  res = a.year - b.year
  if res ~= 0 then return res end
  res = a.month - b.month
  if res ~= 0 then return res end
  return a.day - b.day
end

-- ********************************************************************

while(timecmp(g_start,g_end) <= 0) do
  collect_class(g_start)
  addday(g_start)
end

local keys = {}
for name in pairs(g_categories) do
  table.insert(keys,name)
end

table.sort(keys)

for _,key in ipairs(keys) do
  io.stdout:write(string.format("%s\n",key))
  for _,item in ipairs(g_categories[key]) do
    io.stdout:write(string.format("\t%s\n",item))
  end
end

os.exit(0)
