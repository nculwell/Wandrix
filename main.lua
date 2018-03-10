-- vim: nu et ts=2 sts=2 sw=2

local dbg = require("dbg")
local clr = require("color")

local TICKS_PER_SECOND = 10
local SECS_PER_TICK = 1/TICKS_PER_SECOND
local MOVES_PER_TILE = 5
local START_POS = {x=0,y=0}
local VSYNC = true
local FULLSCREENTYPE = "desktop"
-- local FULLSCREENTYPE = "exclusive"

local glo = {
  fullscreen=true,
  paused=false,
  quitting=false,
  player = {
    pos = START_POS, mov = {x=0,y=0}, draw = {x=0,y=0}
  },
  moveKeys = {}
}

local function dump(z)
  print("{")
  for x in pairs(z) do
    print(x .. ": " .. tostring(z[x]))
  end
  print("}")
end

function love.load()
  print()
  toggleFullscreen()
  love.window.setTitle("Wandrix")
  glo.startTime = love.timer.getTime()
  glo.nextTickTime = 0
  dbg.init("wandrix.log")
  glo.map = load("map")
end

function love.keypressed(key)
  if key == "q" then
    dbg.print("QUITTING")
    love.event.quit()
  elseif key == "space" then
    paused = not paused
  elseif key == "f" then
    toggleFullscreen()
  elseif key == "up" or key == "down" or key == "left" or key == "right" then
    glo.moveKeys[key] = true
  end
end

function toggleFullscreen()
  glo.fullscreen = not glo.fullscreen
  local screenW, screenH, flags = love.window.getMode()
  local newFlags = {
    fullscreen = glo.fullscreen, fullscreentype = FULLSCREENTYPE, vsync = VSYNC, display = flags.display, }
  dump(newFlags)
  print(screenW.."x"..screenH)
  love.window.setMode(screenW, screenH, newFlags)
end

function love.update(dt)
  if paused then return end
  local time = love.timer.getTime() - glo.startTime
  --dbg.print(string.format("%f", time))
  local p = glo.player
  -- logic loop
  while glo.nextTickTime < time do
    dbg.print(string.format("TICK: %f", glo.nextTickTime))
    glo.nextTickTime = glo.nextTickTime + SECS_PER_TICK
    -- Apply previous move
    p.pos.x = p.pos.x + p.mov.x
    p.pos.y = p.pos.y + p.mov.y
    -- Handle next move
    p.mov = scanMoveKeys()
    dbg.print(string.format("MOVE: %f,%f", p.mov.x, p.mov.y))
    -- TODO: Cancel move if invalid.
  end
  local phase = 1 - ((glo.nextTickTime - time) / SECS_PER_TICK)
  p.draw = { x = p.pos.x + phase * p.mov.x, y = p.pos.y + phase * p.mov.y }
end

function scanMoveKeys()
  local move = {x=0, y=0}
  local m = glo.moveKeys
  glo.moveKeys = {}
  if love.keyboard.isDown("left") or m["left"] then move.x = move.x - 1 end
  if love.keyboard.isDown("right") or m["right"] then move.x = move.x + 1 end
  if love.keyboard.isDown("up") or m["up"] then move.y = move.y - 1 end
  if love.keyboard.isDown("down") or m["down"] then move.y = move.y + 1 end
  move.x = move.x / MOVES_PER_TILE
  move.y = move.y / MOVES_PER_TILE
  return move
end

-----------------------------------------------------------
-- DRAW

function love.draw()
  -- Calculations
  local screenSizeX = love.graphics.getWidth()
  local screenSizeY = love.graphics.getHeight()
  local centerX = math.floor(screenSizeX/2)
  local centerY = math.floor(screenSizeY/2)
  local p = glo.player
  local tileW = assert(glo.map.tileSize.w)
  local tileH = assert(glo.map.tileSize.h)
  local pos = { x = p.draw.x * tileW, y = p.draw.y * tileH }
  -- Background
  love.graphics.setColor(clr.GREEN)
  rect("fill", 0, 0, screenSizeX, screenSizeY)
  love.graphics.setColor(clr.WHITE)
  love.graphics.draw(glo.map.image, centerX-pos.x, centerY-pos.y)
  -- Grid lines
  --if true then
  --  love.graphics.setColor(clr.LGREEN)
  --  local tilesPerScreen = { x = math.ceil(screenSizeX / tileW), y = math.ceil(screenSizeY / tileH) }
  --  for gridX = 1, tilesPerScreen.x do
  --    local x = gridX * tileW - ((centerX-pos.x) % tileW)
  --    love.graphics.line(x, 0, x, screenSizeY-1)
  --  end
  --  for gridY = 0, tilesPerScreen.y do
  --    local y = gridY * tileH - ((centerY-pos.y) % tileH)
  --    love.graphics.line(0, y, screenSizeX-1, y)
  --  end
  --end
  -- Player
  love.graphics.setColor(clr.BLUE)
  --dbg.printf("DRAW: %f,%f", pos.x, pos.y)
  local playerSize = (MOVES_PER_TILE-2)/MOVES_PER_TILE
  rect("fill", centerX, centerY, math.floor(playerSize*tileW), math.floor(playerSize*tileW))
end

function rect(mode, x, y, w, h)
  love.graphics.polygon(mode, x, y, x+w, y, x+w, y+h, x, y+h)
end

-----------------------------------------------------------
-- LOAD

function load(filename)
  print("Loading tileset.")
  local tileset = loadTileset(filename)
  print("Loading tiles.")
  local tiles = loadTiles(filename)
  print("Loading image.")
  local image = love.graphics.newImage(filename .. ".png")
  print("Computing tile size.")
  print("Image: " .. image:getWidth() .. " x " .. image:getHeight())
  print("Tiles: " .. table.getn(tiles[1]) .. " x " .. table.getn(tiles))
  local tileSize = tiles.tileSize
  tiles.tileSize = nil
  print(string.format("Tile size: %d x %d", tileSize.w, tileSize.h))
  print("Loading complete.")
  return { tileset=tileset, tiles=tiles, image=image, tileSize=tileSize }
end

local function split(str, delim)
  assert(str)
  assert(delim)
  if not str or string.len(str) == 0 then return {} end
  local pieces = {}
  local i = 1
  local n = 0
  while true do
    n = n + 1
    local f, follow = string.find(str, delim, i, true)
    if f == nil then
      pieces[n] = string.sub(str, i)
      break
    else
      pieces[n] = string.sub(str, i, f-1)
    end
    i = follow + 1
  end
  return pieces
end

function loadTileset(filename)
  local lines = assert(io.lines(filename .. ".tileset"))
  local tileset = {}
  for line in lines do
    local pieces = split(line, ":")
    local name, sound, flags = unpack(pieces)
    tileset[#tileset+1] = { name=name, sound=sound, flags=split(flags, ",") }
  end
  return tileset
end

function loadTiles(filename)
  local lines = assert(io.lines(filename .. ".tiles"))
  local tiles = {}
  local r = -1
  for row in lines do
    r = r + 1
    if r == 0 then
      local tileW, tileH = unpack(split(row, ","))
      tiles.tileSize = {w=tileW,h=tileH}
    else
      tiles[r] = {}
      local rowFields = split(string.sub(row, 2), ",")
      for c, col in ipairs(rowFields) do
        tiles[r][c] = tonumber(col)
        --print("COL " .. r .. " " .. c .. " " .. col)
      end
    end
  end
  return tiles
end

