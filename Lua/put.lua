#!/usr/bin/env lua
-- luacheck: ignore 611

local dump    = require "org.conman.table".dump
local tls     = require "org.conman.net.tls"
local tcp     = require "org.conman.net.tcp"
local url     = require "org.conman.parsers.url"
local magic   = require "org.conman.fsys.magic"
local fsys    = require "org.conman.fsys"
local http    = require "org.conman.parsers.http-client"
local getopt  = require "org.conman.getopt".getopt
local ih      = require "org.conman.parsers.ih"
local base64  = require "org.conman.base64"()
local uurl    = require "url-util"
local status  = require "org.conman.const.http-response"
local zlib    = require "zlib"

local headers = ih.headers(ih.generic)
local gf_debug

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

local function readbody(conn,hdrs)
  local body
  
  if hdrs['Content-Length'] then
    body = conn:read(hdrs['Content-Length'])
  elseif hdrs['Transfer-Encoding'] == 'chunked' then
    body = ""
    repeat
      local line = conn:read("l")
      if not line then break end
      local len  = tonumber(line,16)
      if not len then break end
      body       = body .. (conn:read(len) or "")
      conn:read("l")
    until len == 0
  else
    body = ""
  end
  
  if hdrs['Content-Encoding'] == 'gzip' then
    local s = zlib.inflate(body)
    body    = s:read("*a")
  end
  
  return body
end

-- ***********************************************************************

local function get(conn,location)
  local path do
    if location.query then
      path = string.format("%s?%s",location.path,location.query)
    else
      path = location.path
    end
  end
  
  conn:write(
          string.format("GET %s HTTP/1.1\r\n",path),
          string.format("Host: %s:%d\r\n",location.host,location.port),
          "User-Agent: mod-blog-updater/1.0\r\n",
          "Accept: text/plain\r\n",
          "\r\n"
  );
  conn:flush()
  
  local response = conn:read("l")
  local hdrs     = conn:read("h")
  
  response   = http.response:match(response)
  hdrs       = http.headers:match(hdrs)
  local body = readbody(conn,hdrs)
  
  if gf_debug then
    dump("GET response",response)
    dump("GET headers",hdrs)
    dump("GET body",body)
  end
  
  if response.status == status.SUCCESS.OKAY then
    return body,response.status
  else
    return nil,response.status
  end
end

-- ***********************************************************************

local function put(conn,location,hdrs,mimetype,body)
  conn:write(
          string.format("PUT %s HTTP/1.1\r\n",location.path),
          string.format("Host: %s:%d\r\n",location.host,location.port),
          "User-Agent: mod-blog-updater/1.0\r\n",
          string.format("Content-Type: %s\r\n",mimetype),
          string.format("Content-Length: %d\r\n",#body),
          "Accept: */*\r\n"
  )
  
  for name,val in pairs(hdrs) do
    conn:write(string.format("%s: %s\r\n",name,val))
  end
  
  conn:write("\r\n",body)
  conn:flush()
  
  local r = conn:read("l")
  local h = conn:read("h")
  
  local res  = http.response:match(r)
  local hdr  = http.headers:match(h)
  body       = readbody(conn,hdr)
  
  if gf_debug then
    dump("PUT response",res)
    dump("PUT headers",hdr)
    dump("PUT body",body)
  end
  
  return res.status >= 200 and res.status <= 299 , res.status
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
    errmsg("%s: not a proper entry")
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
        -d | --debug
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
  { "b" , "blog"  , true  , function(b) BLOG = b        end },
  { "d" , "debug" , false , function()  gf_debug = true end },
  { "h" , "help"  , false , function()  usage()         end },
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
    os.exit(errmsg("%s: missing %q config block",CONF,BLOG))
  end
  
  BASEURL = confenv[BLOG].url
  
  if not BASEURL then
    os.exit(errmsg("%s: missing %s.url directive",CONF,BLOG))
  end
  
  USER = confenv[BLOG].user
  if USER then
    USER = "Basic " .. base64:encode(USER)
  end
end

-- ------------------------------------------------------------------

if gf_debug then
  io.stderr:write(string.format([[
base-url=%q
entry-file=%q
]],BASEURL,arg[fi]))
end

local body,date = loadentry(arg[fi])

if not body then
  os.exit(1,true)
end

local base     = url:match(BASEURL)
local path     = url:match(date .. '/' .. fsys.basename(arg[fi]))
local location = uurl.merge(base,path)

if gf_debug then
  io.stderr:write(string.format("location: %s body: %d\n",uurl.toa(location),#body))
end

local conn,err do
  if location.scheme == 'https' then
    conn,err = tls.connect(location.host,location.port)
  else
    conn,err = tcp.connect(location.host,location.port)
  end
end

if not conn then
  os.exit(errmsg("%s: %s",location.host,err))
end

local newurl do
  local basen = url:match(BASEURL)
  local pathn = url:match("/boston.cgi?cmd=last&date="..date)
  local locationn = uurl.merge(basen,pathn)
  newurl = get(conn,locationn)
  
  local front,part = newurl:match("^(.*)(%d+)$")
  part = tonumber(part) + 1
  newurl = front .. part
end

if gf_debug then
  io.stderr:write(string.format("newurl=%q\n",newurl))
end

location = url:match(newurl)

local http_headers = {} do
  if USER then
    http_headers['Authorization'] = USER
  end
  
  if fi == #arg then
    http_headers['Connection'] = 'close'
  end
  
  if gf_debug then
    dump("headers",http_headers)
  end
end

msg("PUT %s (%d)",uurl.toa(location),#body)
if not put(conn,location,http_headers,"application/mod_blog",body) then
  conn:close()
  errmsg("Failed: PUT %s",uurl.toa(location))
  errmsg("\tForgot credentials?")
  os.exit(1,true)
end

for i = fi + 1 , #arg do
  local f,err2 = io.open(arg[i],"rb")
  if f then
    local bodyf  = f:read("*a")
    f:close()
    
    local pathf     = url:match(date .. '/' .. fsys.basename(arg[i]))
    local locationf = uurl.merge(base,pathf)
    
    if gf_debug then
      io.stderr:write(string.format(
        "location: %s body: %d mime: %s\n",
        uurl.toa(locationf),
        #bodyf,
        magic(bodyf,true)
      ))
    end
    
    local http_headersf = { ['Blog-File'] = true }
    
    if USER then
      http_headersf['Authorization'] = USER
    end
    
    if i == #arg then
      http_headersf['Connection'] = 'close'
    end
    
    if gf_debug then
      dump("headers",http_headersf)
    end
    
    msg("PUT %s (%d)",uurl.toa(locationf),#bodyf)
    put(conn,locationf,http_headersf,magic(bodyf,true),bodyf)
  else
    errmsg("%s: %s",arg[i],err2)
  end
end

conn:close()
