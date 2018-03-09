-- vim: nu et ts=2 sts=2 sw=2

local dbg = {}
local filename = "debug.log"

function dbg.init(logFilename)
  if logFilename ~= nil then
    filename = logFilename
  end
  local file = io.open(filename, "w")
  file:write("STARTING\n")
  file:close()
end

function dbg.print(message)
  local file = io.open(filename, "a")
  file:write(message)
  file:write("\n")
  file:close()
end

function dbg.printf(fmt, ...)
  local message = string.format(fmt, unpack(arg))
  dbg.print(message)
end

return dbg

