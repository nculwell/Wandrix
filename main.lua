-- vim: nu et ts=2 sts=2 sw=2

local dbg = require("dbg")

local SCREEN_MODE = {800,600}
local TILE_SIZE = {w=100,h=100}
local TICKS_PER_SECOND = 10
local MOVES_PER_GRID_SQUARE = 5
local GRID_SQUARES_PER_SCREEN = {x=8,y=6}
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
  love.window.setMode(SCREEN_MODE[1], SCREEN_MODE[2],
    { fullscreen = glo.fullscreen })
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
  move.x = move.x / MOVES_PER_GRID_SQUARE
  move.y = move.y / MOVES_PER_GRID_SQUARE
  return move
end

function love.draw()
  -- Calculations
  local screenSizeX = love.graphics.getWidth()
  local screenSizeY = love.graphics.getHeight()
  local centerX = math.floor(screenSizeX/2)
  local centerY = math.floor(screenSizeY/2)
  local gridSquareSize = {
    x = screenSizeX / GRID_SQUARES_PER_SCREEN.x,
    y = screenSizeY / GRID_SQUARES_PER_SCREEN.y
  }
  local p = glo.player
  local pos = { x = p.draw.x * gridSquareSize.x, y = p.draw.y * gridSquareSize.y }
  -- Background
  love.graphics.setColor(GREEN)
  rect("fill", 0, 0, screenSizeX, screenSizeY)
  love.graphics.draw(glo.map.image, -pos.x, -pos.y)
  -- Grid lines
  love.graphics.setColor(LGREEN)
  for gridX = 1, GRID_SQUARES_PER_SCREEN.x do
    local x = gridX * gridSquareSize.x - (pos.x % gridSquareSize.x)
    love.graphics.line(x, 0, x, screenSizeY-1)
  end
  for gridY = 0, GRID_SQUARES_PER_SCREEN.y do
    local y = gridY * gridSquareSize.y - (pos.y % gridSquareSize.y)
    love.graphics.line(0, y, screenSizeX-1, y)
  end
  -- Player
  love.graphics.setColor(BLUE)
  dbg.printf("DRAW: %f,%f", pos.x, pos.y)
  rect("fill", centerX, centerY, .6*gridSquareSize.x, .6*gridSquareSize.y)
end

function rect(mode, x, y, w, h)
  love.graphics.polygon(mode, x, y, x+w, y, x+w, y+h, x, y+h)
end

function load(filename)
  local tileset = loadTileset(filename)
  local tiles = loadTiles(filename)
  local image = love.graphics.newImage(filename + ".png")
  return { tileset=tileset, tiles=tiles, image=image }
end

local function split(str, delim)
  if not str or string.len(str) == 0 then return {} end
  local pieces = {}
  local i = 1
  while true do
    local f = string.find(str, delim, i, true)
    if f == nil then
      pieces[#pieces+1] = string.sub(str, i)
      break
    else
      pieces[#pieces+1] = string.sub(str, i, f-i)
    end
  end
  return pieces
end

function loadTileset(filename)
  local f = io.open(filename + ".ts", "rt")
  local tileset = {}
  for ln in f:lines() do
    local pieces = split(ln, ":")
    local name, sound, flags = unpack(pieces)
    tileset[#tileset+1] = { name=name, sound=sound, flags=split(flags, ",") }
  end
  return tileset
end

function loadTiles(filename)
  local f = io.open(filename + ".dat", "rb")
  local buffer = f:read("*all")
  local headers, offset = readInts(buffer, 0, 4)
  local tileDimensions  = { w=headers[1], h=headers[2] }
  local tiles = {}
  for y = 1, tileDimensions.h do
    tiles[y], offset = readInts(buffer, offset, tileDimensions.w)
  end
  return tiles
end

function readInts(buffer, offset, count, intWidthBytes)
  if intWidthBytes == nil then intWidthBytes = 4 end
  local ints = {}
  for i = 1, count do
    local n = 0
    for byte = 1, intWidthBytes do
      n = n | (buffer[offset] << (4-byte))
      offset = offset + 1
    end
    ints[i] = n
  end
  return ints, offset
end

