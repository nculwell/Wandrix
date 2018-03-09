-- vim: nu et ts=2 sts=2 sw=2

local dbg = require("dbg")
local clr = require("color")

local SCREEN_MODE = {800,600}
local TILE_SIZE = {w=100,h=100}
local TICKS_PER_SECOND = 10
local MOVES_PER_TILE = 5
local MAP_SIZE = {w=2000,h=2000}
local START_POS = {x=1000,y=1000}

local glo = {
  fullscreen=true,
  paused=false,
  quitting=false,
  startTime=0,
  nextTickTime=0,
  player = {
    pos = {x=0,y=0}, mov = {x=0,y=0}, draw = {x=0,y=0}
  },
  moveKeys = {}
}

function dump(z)
  print("{")
  for x in pairs(z) do
    print(x .. ": " .. tostring(z[x]))
  end
  print("}")
end

function love.load()
  toggleFullscreen()
  love.window.setTitle("Wandrix")
  glo.startTime = love.timer.getTime()
  glo.nextTickTime = 0
  dump(dbg)
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
  love.window.setMode(SCREEN_MODE[1], SCREEN_MODE[2], { fullscreen = glo.fullscreen })
end

function love.update(dt)
  if paused then return end
  local time = love.timer.getTime() - glo.startTime
  --dbg.print(string.format("%f", time))
  local p = glo.player
  -- logic loop
  while glo.nextTickTime < time do
    dbg.print(string.format("TICK: %f", glo.nextTickTime))
    glo.nextTickTime = glo.nextTickTime + (1 / TICKS_PER_SECOND)
    -- Apply previous move
    p.pos.x = p.pos.x + p.mov.x
    p.pos.y = p.pos.y + p.mov.y
    -- Handle next move
    p.mov = scanMoveKeys()
    dbg.print(string.format("MOVE: %f,%f", p.mov.x, p.mov.y))
    -- TODO: Cancel move if invalid.
  end
  -- current phase display
  local phase = 1 - ((glo.nextTickTime - time) / (1 / TICKS_PER_SECOND))
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
  love.graphics.draw(glo.map.image, -pos.x, -pos.y)
  -- Grid lines
  if true then
    love.graphics.setColor(clr.LGREEN)
    local tilesPerScreen = { x = math.ceil(screenSizeX / tileW), y = math.ceil(screenSizeY / tileH) }
    for gridX = 1, tilesPerScreen.x do
      local x = gridX * tileW - (pos.x % tileW)
      love.graphics.line(x, 0, x, screenSizeY-1)
    end
    for gridY = 0, tilesPerScreen.y do
      local y = gridY * tileH - (pos.y % tileH)
      love.graphics.line(0, y, screenSizeX-1, y)
    end
  end
  -- Player
  love.graphics.setColor(clr.BLUE)
  --dbg.printf("DRAW: %f,%f", pos.x, pos.y)
  rect("fill", centerX, centerY, .6*tileW, .6*tileW)
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
  local tileW = image:getWidth() / table.getn(tiles[1])
  local tileH = image:getHeight() / table.getn(tiles)
  print("Tile size: " .. tileW .. " x " .. tileH)
  local tileSize = { w = tileW, h = tileH }
  print(string.format("Tile size: %d x %d", tileSize.w, tileSize.h))
  print("Loading complete.")
  return { tileset=tileset, tiles=tiles, image=image, tileSize=tileSize }
end

local function split(str, delim)
  assert(str)
  assert(delim)
  --print(str)
  if not str or string.len(str) == 0 then return {} end
  local pieces = {}
  local i = 1
  local n = 0
  while true do
    n = n + 1
    local f, follow = string.find(str, delim, i, true)
    --if f then print("f="..f) end
    --if follow then print("follow="..follow) end
    if f == nil then
      --print(i .. ",end")
      pieces[n] = string.sub(str, i)
      break
    else
      --print(i..","..f.."="..string.sub(str,i,f-1))
      pieces[n] = string.sub(str, i, f-1)
    end
    --print(pieces[n])
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
  local r = 0
  for row in lines do
    r = r + 1
    tiles[r] = {}
    local rowFields = split(string.sub(row, 2), ",")
    for c, col in ipairs(rowFields) do
      tiles[r][c] = tonumber(col)
      --print("COL " .. r .. " " .. c .. " " .. col)
    end
    print("" .. table.getn(tiles[r]))
  end
  return tiles
end

