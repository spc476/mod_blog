#!/usr/bin/env Lua
-- GPL2+ Copyright 2026 by Sean Conner
-- luacheck: ignore 611
-- ************************************************************************
-- for PUT:
--	201 Created
--	202 Accepted		entry-post-hook failed
--	400 Bad Request
--	401 Unauthorized
--	408 Timed out
--	422 Unprocess Entiry	entry-pre-hook failed
--	500 Internal Server Error
-- ************************************************************************

local tls         = require "org.conman.net.tls"
local tcp         = require "org.conman.net.tcp"
local url         = require "org.conman.parsers.url"
local magic       = require "org.conman.fsys.magic"
local fsys        = require "org.conman.fsys"
local http        = require "org.conman.parsers.http-client"
local getopt      = require "org.conman.getopt".getopt
local ih          = require "org.conman.parsers.ih"
local base64      = require "org.conman.base64"()
local uurl        = require "url-util"
local http_status = require "org.conman.const.http-response"
local zlib        = require "zlib"

local headers     = ih.headers(ih.generic)

magic:flags('mime')

-- ************************************************************************

local function errmsg(...)
  io.stderr:write(string.format(...),'\n')
  return 1
end

-- ************************************************************************

local function msg(...)
  io.stdout:write(string.format(...),'\n')
end

-- ************************************************************************

local function request(conn,method,location,closef,user,mimetype,body)
  local path do
    if location.query then
      path = string.format("%s?%s",location.path,location.query)
    else
      path = location.path
    end
  end
  
  conn:write(
          string.format("%s %s HTTP/1.1\r\n",method,path),
          string.format("Host: %s:%d\r\n",location.host,location.port),
          "User-Agent: mod-blog-updater/1.2\r\n",
          "Accept: */*\r\n"
  )
  
  if mimetype then
    conn:write(
            string.format("Content-Type: %s\r\n",mimetype),
            string.format("Content-Length: %d\r\n",#body)
    )
  end
  
  if closef then
    conn:write("Connection: close\r\n")
  end
  
  if user then
    conn:write(string.format("Authorization: %s\r\n",user))
  end
  
  conn:write("\r\n")
  conn:flush()
  
  if body then
    while #body > 0 do
      local data = body:sub(1,1024)
      conn:write(data)
      conn:flush()
      body = body:sub(1025,-1)
    end
  end
    
  local r   = conn:read("l")
  local h   = conn:read("h")
  local res = http.response:match(r)
  local hdr = http.headers:match(h)
  
  if hdr['Content-Length'] then
    body = conn:read(hdr['Content-Length'])
  elseif hdr['Transfer-Encoding'] == 'chunked' then
    body = ""
    repeat
      local line = conn:read("l")
      if not line then break end
      local len = tonumber(line,16)
      if not len then break end
      body = body .. (conn:read(len) or "")
      conn:read("l")
    until len == 0
  else
    body = ""
  end
  
  if hdr['Content-Encoding'] == 'gzip' then
    local s = zlib.inflate(body)
    body    = s:read("*a")
  end
  
  return res.status,body
end

-- ************************************************************************

local function loadentry(filename)
  local f,err = io.open(filename,"r")
  if not f then
    errmsg("%s: %s",filename,err)
    return
  end
  
  local body = f:read("*a")
  f:close()
  
  if #body == 0 then
    errmsg("%s: empty",filename);
    return
  end
  
  local hdrs = headers:match(body)
  
  if not hdrs then
    errmsg("%s: not a proper entry",filename)
    return
  end
  
  if not hdrs['Date'] then
    hdrs['Date'] = os.date("%Y/%m/%d")
  end
  
  return body,hdrs['Date']
end

-- ************************************************************************

local function usage()
  io.stderr:write(string.format([[
usage: %s [options] entryfile [files...]
	-b | --blog blogref
	-h | --help
]],arg[0]))
  os.exit(1,true)
end

-- ************************************************************************

local CONF = "/home/spc/.config/mod-blog-put.config"
local BLOG = 'default'
local BASEURL
local USER
local opts =
{
  { "b" , "blog"  , true  , function(b) BLOG = b end },
  { "h" , "help"  , false , function()  usage()  end },
}

local fi = getopt(arg,opts)

if not arg[fi] then
  os.exit(errmsg("missing entry file"),true)
end

do
  local confenv = {}
  local conf,err = loadfile(CONF,"t",confenv)
  if not conf then
    os.exit(errmsg("%s: %s",CONF,err),true)
  end

  conf()

  if not confenv[BLOG] then
    os.exit(errmsg("%s: missing %q config block",CONF,BLOG),true)
  end

  BASEURL = confenv[BLOG].url

  if not BASEURL then
    os.exit(errmsg("%s: missing %s.url directive",CONF,BLOG),true)
  end

  USER = confenv[BLOG].user
  if USER then
    USER = "Basic " .. base64:encode(USER)
  end
end

-- ************************************************************************

local body,date = loadentry(arg[fi])

if not body then
  os.exit(1,true)
end

local base     = url:match(BASEURL)
local path     = url:match(date .. '/' .. fsys.basename(arg[fi]))
local location = uurl.merge(base,path)

local conn,err do
  if location.scheme == 'https' then
    conn,err = tls.connect(location.host,location.port)
  else
    conn,err = tcp.connect(location.host,location.port)
  end
end

if not conn then
  os.exit(errmsg("%s: %s",location.host,err),true)
end

-- --------------------------------
-- Get next entry URL
-- --------------------------------

local newurl do
  local status,bodyg
  local basen = url:match(BASEURL)
  local pathn = url:match("/boston.cgi?cmd=last&date="..date)
  location    = uurl.merge(basen,pathn)
  
  msg("GET '%s'",uurl.toa(location))
  status,bodyg = request(conn,"GET",location)
  
  if status ~= http_status.SUCCESS.OKAY then
    os.exit(errmsg("%d `%s`",status,uurl.toa(location)),true)
  end
  
  local front,part = bodyg:match("^(.*)(%d+)$")
  part   = tonumber(part) + 1
  newurl = front .. part
end

location = url:match(newurl)

-- ------------------------------------------
-- PUT the new entry
-- ------------------------------------------

msg("PUT '%s' (%d)",uurl.toa(location),#body)
local status = request(conn,"PUT",location,fi == #arg,USER,"application/mod_blog",body)
if status ~= http_status.SUCCESS.CREATED then
  os.exit(errmsg("PUT: %d '%s'",status,uurl.toa(location)),true)
end

-- -----------------------------------
-- PUT any other files specified
-- -----------------------------------

local excode = 0

for i = fi + 1 , #arg do
  local f,err2 = io.open(arg[i],"rb")
  if f then
    body = f:read("*a")
    f:close()
    
    local pathf     = url:match(date .. '/' .. fsys.basename(arg[i]))
    local locationf = uurl.merge(base,pathf)
    
    msg("PUT '%s' (%d)",uurl.toa(locationf),#body)
    status = request(conn,"PUT",locationf,i==#arg,USER,magic(body,true),body)
    
    if status ~= http_status.SUCCESS.CREATED then
      excode = errmsg("PUT: %d '%s'",status,uurl.toa(locationf))
    end
  else
    excode = errmsg("%s: %s",arg[i],err2)
  end
end

os.exit(excode,true)
