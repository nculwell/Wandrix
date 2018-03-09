-- vim: nu et ts=2 sts=2 sw=2

local generate = require("generate")
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
  dump(generate)
  dbg.init("wandrix.log")
  glo.map = generate.newMap(MAP_SIZE, TILE_SIZE)
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
  local f = io.open(filename, "rb")
  local buffer = f:read("*all")
  local headers, offset = readInts(buffer, 0, 4)
  local tileDimensions  = { w=headers[1], h=headers[2] }
  local pixelDimensions = { w=headers[3], h=headers[4] }
  local tiles = {}
  for y = 1 to tileDimensions.h do
    tiles[y], offset = readInts(buffer, offset, tileDimensions.w)
  end
  local img = love.image.new
  for y = 1 to pixelDimensions.h do
    pixels[y] = {}
    for x = 1 to pixelDimensions.w do
      local pixel = buffer[offset]
      offset = offset + 1
      pixels[y][x] = convert8BitColor(pixel)
    end
  end
  return { tiles=tiles, pixels=pixel }
end

function convert8BitColor(c)
  local r = (pixel & (32|16)) >> 4
  local b = (pixel & (8|4)) >> 2
  local g = (pixel & (2|1))
  local color = {}
  for i = 1, 3 do
    x = {r,g,b}[i]
    local y
    if x == 3 then y = 255
    elseif x == 2 then y = 127
    elseif x == 1 then y = 63
    else y = 0
    end
    color[i] = y
  end
  return color
end

function readInts(buffer, offset, count)
  local ints = {}
  for i = 1, count do
    local n = 0
    for byte = 1, 4 do
      n = n | (buffer[offset] << (4-byte))
      offset = offset + 1
    end
    ints[i] = n
  end
  return ints, offset
end

