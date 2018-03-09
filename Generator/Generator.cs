using SixLabors.ImageSharp;
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

        private Image<Rgba32> GenerateImage(List<Tile> ts, int[,] tiles)
        {
            var mapBox = new Box(tiles);
            int MaxRadius = Math.Min(_tileSize.W, _tileSize.H) / 3;
            var imageSize = _mapSize * _tileSize;
            var image = new Image<Rgba32>(imageSize.H, imageSize.H);
            for (int y = 0; y < imageSize.H; ++y)
            {
                for (int x = 0; x < imageSize.W; ++x)
                {
                    var colors = new List<Rgb>(ts[tiles[x / _tileSize.W, y / _tileSize.H]].Colors);
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
                    image[x, y] = pixel.ToRgba32();
                }
            }
            return image;
        }

    }
}
