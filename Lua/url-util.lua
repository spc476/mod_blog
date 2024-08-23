-- ***************************************************************
--
-- Copyright 2019 by Sean Conner.  All Rights Reserved.
--
-- This library is free software; you can redistribute it and/or modify it
-- under the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 3 of the License, or (at your
-- option) any later version.
--
-- This library is distributed in the hope that it will be useful, but
-- WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
-- License for more details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this library; if not, see <http://www.gnu.org/licenses/>.
--
-- Comments, questions and criticisms can be sent to: sean@conman.org
--
-- ********************************************************************
-- luacheck: globals rm_dot_segs merge toa query toq
-- luacheck: globals esc_auth esc_path esc_query esc_frag
-- luacheck: ignore 611

local gtypes = require "org.conman.const.gopher-types"
local lpeg   = require "lpeg"
local string = require "string"
local table  = require "table"

local tonumber = tonumber
local tostring = tostring
local type     = type
local pairs    = pairs

_ENV = {}

local Cc = lpeg.Cc
local Cf = lpeg.Cf
local Cg = lpeg.Cg
local Cs = lpeg.Cs
local C  = lpeg.C
local P  = lpeg.P
local R  = lpeg.R

-- ********************************************************************
-- Note: The commented letters A, B, C, etc., reference RFC-3986 5.2.4.
-- ********************************************************************

local segment = P'../'         * Cc''        * Cc(false) -- A
              + P'./'          * Cc''        * Cc(false)
              + P'/.'  * #P'/' * Cc''        * Cc(false) -- B
              + P'/.'  * P(-1) * Cc'/'       * Cc(false)
              + P'/..' * #P'/' * Cc''        * Cc(true)  -- C
              + P'/..' * P(-1) * Cc'/'       * Cc(true)
              + P'..'  * P(-1) * Cc''        * Cc(false) -- D
              + P'.'   * P(-1) * Cc''        * Cc(false)
              + C(P'/'^-1 * (P(1) - P'/')^1) * Cc(false) -- E
              + C'/'                         * Cc(false)
              
rm_dot_segs = Cf(Cc"" * (Cg(segment))^1,function(acc,capture,trim)
  if trim then
    for i = -2,-#acc,-1 do
      if acc:sub(i,i) == '/' then
        return acc:sub(1,i-1) .. capture
      end
    end
    return capture
  else
    return acc .. capture
  end
end) + Cc''

-- ********************************************************************
-- Usage:       u = merge(Base,R)
-- Desc:        Merge a URL with a base URL
-- Input:       Base (table) a URL from url2:match()
--              R (table) a URL from url2:match()
-- Return:      u (table) A merged URL
--
-- Note:        Implements the algorithm from RFC-3986 5.2.2, with the
--              "authority" portion being .host, .port and .user.
-- ********************************************************************

function merge(Base,Ru)
  local function mergepath()
    if Base.host and Base.path == "" then
      return "/" .. Ru.path
    else
      for i = -1 , -#Base.path , -1 do
        if Base.path:sub(i,i) == '/' then
          return Base.path:sub(1,i) .. Ru.path
        end
      end
    end
  end
  
  local T = {}
  
  if Ru.scheme then
    T.scheme = Ru.scheme
    T.host   = Ru.host -- authority
    T.port   = Ru.port
    T.user   = Ru.user
    T.path   = Ru.path
    T.path   = rm_dot_segs:match(T.path)
    T.query  = Ru.query
  else
    if Ru.host then
      T.host  = Ru.host
      T.port  = Ru.port
      T.user  = Ru.user
      T.path  = rm_dot_segs:match(Ru.path)
      T.query = Ru.query
    else
      if Ru.path == "" then
        T.path = Base.path
        if Ru.query then
          T.query = Ru.query
        else
          T.query = Base.query
        end
      else
        if Ru.path:match "^/" then
          T.path = rm_dot_segs:match(Ru.path)
        else
          T.path = mergepath()
          T.path = rm_dot_segs:match(T.path)
        end
        T.query = Ru.query
      end
      T.host = Base.host
      T.port = Base.port
      T.user = Base.user
    end
    T.scheme = Base.scheme
  end
  T.fragment = Ru.fragment
  return T
end

-- ********************************************************************

local function tohex(c)
  return string.format("%%%02X",string.byte(c))
end

local unsafe     = P" "  / "%%20"
                 + P"#"  / "%%23"
                 + P"%"  / "%%25"
                 + P"<"  / "%%3C"
                 + P">"  / "%%3E"
                 + P"["  / "%%5B"
                 + P"\\" / "%%5C"
                 + P"]"  / "%%5D"
                 + P"^"  / "%%5E"
                 + P"{"  / "%%7B"
                 + P"|"  / "%%7C"
                 + P"}"  / "%%7D"
                 + P'"'  / "%%22"
                 + R("\0\31","\127\255") / tohex
local char_auth  = P"?"  / "%%3F"
                 + P"@"  / "%%40"
                 + unsafe
                 + P(1)
local char_path  = P"?"  / "%%3F"
                 + unsafe
                 + P(1)
local char_query = P"="  / "%%3D"
                 + P"&"  / "%%26"
                 + unsafe
                 + P(1)
local char_frag  = unsafe
                 + P(1)
esc_auth         = Cs(char_auth^0)
esc_path         = Cs(char_path^0)
esc_query        = Cs(char_query^0)
esc_frag         = Cs(char_frag^0)

-- ********************************************************************

local xdigit = lpeg.locale().xdigit
local char   = lpeg.P"%" * lpeg.C(xdigit * xdigit)
             / function(c)
                 return string.char(tonumber(c,16))
               end
             + lpeg.R"!~"
local name   = lpeg.Cs((char - (lpeg.P"=" + lpeg.P"&"))^1)
local value  = lpeg.P"=" * lpeg.Cs((char - lpeg.P"&")^1)
             + lpeg.Cc(true)
             
query        = lpeg.Cf(
                        lpeg.Ct"" * lpeg.Cg(name * value * lpeg.P"&"^-1)^0,
                         function(result,n,v)
                           if result and not result[n] then
                             result[n] = v
                             return result
                           else
                             return false
                           end
                         end
                       )
                       
-- ********************************************************************

function toq(q)
  local res = {}
  for n,v in pairs(q) do
    if v == true then
      table.insert(res,esc_query:match(n))
    else
      table.insert(res,string.format("%s=%s",
          esc_query:match(tostring(n)),
          esc_query:match(tostring(v))
      ))
    end
  end
  return table.concat(res,"&")
end

-- ********************************************************************
-- Note:        From RFC-3986 5.3
-- ********************************************************************

local SCHEMES =
{
  http   =   80,
  https  =  443,
  ftp    =   21,
  gemini = 1965,
}

function toa(u)
  local authority
  
  if u.host then
    authority = ""
    if u.user then authority = esc_auth:match(u.user) .. "@" end
    if u.host:match"%:" then
      authority = authority .. "[" .. esc_auth:match(u.host) .. "]"
    else
      authority = authority .. esc_auth:match(u.host)
    end
    if u.port and u.port ~= SCHEMES[u.scheme] then
      authority = authority .. ':' .. tostring(u.port)
    end
  end
  
  local result = ""
  if u.scheme  then result = result .. u.scheme .. ':'   end
  if authority then result = result .. "//" .. authority end

  if u.scheme == 'gopher' then
    result = result .. "/" .. gtypes[u.type] .. esc_path:match(u.selector)
    if u.search then
      result = result .. "%09" .. toq(u.search)
    end
  else
    result = result .. esc_path:match(u.path)
                    
    if u.query then
      if type(u.query) == 'table' then
        result = result .. "?" .. toq(u.query)
      else
        result = result .. "?" .. u.query
      end
    end
    
    if u.fragment  then result = result .. "#" .. esc_frag:match(u.fragment) end
  end
  return result
end

-- ********************************************************************

return _ENV
