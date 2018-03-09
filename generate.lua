-- vim: nu et ts=2 sts=2 sw=2

local c = require("color")

local generate = {}

local function _tileset()
  -- TODO: Load from file.
  local tileset = {
    { name="water", palette={ c.WHITE, c.LGREEN, c.BLUE, c.BLUE, c.LBLUE, c.BLACK } },
    { name="grass", palette={ c.RED, c.GREEN, c.GREEN, c.LGREEN, c.YELLOW } },
    { name="field", palette={ c.RED, c.ORANGE, c.BLACK, c.YELLOW, c.YELLOW } },
    { name="trees", palette={ c.RED, c.GREEN, c.GREEN, c.BLUE, c.LGREEN } },
  }
  return tileset
end

-- TODO: Sensible generation algorithm
local function _tiles(mapSize, tileset)
  local tiles = {}
  for y = 1, mapSize.h do
    tiles[y] = {}
    for x = 1, mapSize.w do
      local tileId = math.random(table.getn(tileset))
      tiles[y][x] = tileId
    end
  end
  local map = { tiles=tiles, tileset=tileset }
  function map:tileAt(x, y)
    local xy
    if y == nil then xy = x
    else xy = { x=x, y=y }
    end
    xy = { x = math.floor(xy.x), y = math.floor(xy.y) }
    print(xy.x)
    print(xy.y)
    local row = self.tiles[xy.y]
    local t = row and row[xy.x]
    return t and self.tileset[t]
  end
  return map
end

local function concatTable(t1, t2)
    for i= 1, #t2 do
        t1[#t1+1] = t2[i]
    end
    return t1
end

local function _image(map, tileSize)
  local N_POINTS = 5
  local MAX_RADIUS = math.min(tileSize.w, tileSize.h) / 3
  local tiles = map.tiles
  local ts = map.tileset
  local img = love.image.newImageData(table.getn(tiles[1]) * tileSize.w, table.getn(tiles) * tileSize.h)
  for y = 1, table.getn(tiles) do
    for x = 1, table.getn(tiles[1]) do
      local colors = concatTable({}, map:tileAt(x, y).palette)
      for p = 1, N_POINTS do
        local radius = math.random() * MAX_RADIUS
        local angle = math.random() * 2 * math.pi
        local offsetX = radius * math.cos(angle)
        local offsetY = radius * math.sin(angle)
        local pointTile = map:tileAt(x + offsetX, y + offsetY)
        if pointTile then
          colors = concatTable(colors, pointTile.palette)
        end
      end
      local colorIndex = math.random(table.getn(colors))
      local pixel = colors[colorIndex]
      img:setPixel(x-1, y-1, unpack(pixel))
    end
  end
  return {
    tiles = tiles,
    tileset = tileset,
    image = love.graphics.newImage(img)
  }
end

function generate.newMap(mapSize, tileSize)
  local tileset = _tileset()
  local map = _tiles(mapSize, tileset)
  local map = _image(map, tileSize)
  return map
end

return generate

