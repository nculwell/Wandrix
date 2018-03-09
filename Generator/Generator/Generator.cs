using System;
using System.Collections.Generic;
using System.Text;

namespace Generator
{
    class Generator
    {
        readonly Random r = new Random();
        readonly Size _mapSize, _tileSize;
        const int NPoints = 5;

        private Generator(Size mapSize, Size tileSize)
        {
            _mapSize = mapSize;
            _tileSize = tileSize;
        }

        public static Map NewMap(Size mapSize, Size tileSize)
        {
            return new Generator(mapSize, tileSize).Generate();
        }

        private Map Generate()
        {
            var ts = Tile.Tileset();
            var tiles = GenerateTiles(ts.Count);
            var image = GenerateImage(ts, tiles);
            Map m = new Map() { Tileset = ts, Tiles = tiles, Image = image };
            return m;
        }

        // TODO: Better algorithm for this.
        private int[,] GenerateTiles(int nTiles)
        {
            var tiles = new int[_mapSize.W, _mapSize.H];
            for (int y = 0; y < _mapSize.H; ++y)
            {
                for (int x = 0; x < _mapSize.W; ++x)
                {
                    int tileId = r.Next(nTiles);
                    tiles[y, x] = tileId;
                }
            }
            return tiles;
        }

        private Rgb[,] GenerateImage(List<Tile> ts, int[,] tiles)
        {
            var mapBox = new Box(tiles);
            int MaxRadius = Math.Min(_tileSize.W, _tileSize.H) / 3;
            var imageSize = _mapSize * _tileSize;
            var image = new Rgb[imageSize.W, imageSize.H];
            for (int y = 0; y < _mapSize.H; ++y)
            {
                for (int x = 0; x < _mapSize.W; ++x)
                {
                    var colors = new List<Rgb>(ts[tiles[x, y]].Colors);
                    for (int p = 0; p < NPoints; ++p)
                    {
                        double radius = r.NextDouble() * MaxRadius;
                        double angle = r.NextDouble() * 2 * Math.PI;
                        int offsetX = (int)Math.Floor(radius * Math.Cos(angle));
                        int offsetY = (int)Math.Floor(radius * Math.Sin(angle));
                        var tileXY = new Point((x + offsetX) / _tileSize.W, (y + offsetY) / _tileSize.H);
                        if (mapBox.Contains(tileXY))
                        {
                            int neighborTileIndex = tiles[tileXY.X, tileXY.Y];
                            Tile t = ts[neighborTileIndex];
                            colors.AddRange(t.Colors);
                        }
                    }
                    int colorIndex = r.Next(colors.Count);
                    Rgb pixel = colors[colorIndex];
                    image[x, y] = pixel;
                }
            }
            return image;
        }

    }
}

/*
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
*/

