using SixLabors.ImageSharp;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;

namespace Generator
{
    class Map
    {
        public List<Tile> Tileset;
        public int[,] Tiles;
        public Image<Rgba32> Image;
        private Size _tileSize;

        public Map(Size tileSize) { _tileSize = tileSize; }

        public void Save(string filename)
        {
            using (var f = new StreamWriter(filename + ".tileset"))
                foreach (var t in Tileset)
                    f.WriteLine(t.ToFileRepr());
            using (var f = new StreamWriter(filename + ".tiles"))
            {
                f.WriteLine($"{_tileSize.W},{_tileSize.H}");
                for (int y = 0; y < Tiles.GetLength(1); ++y)
                {
                    for (int x = 0; x < Tiles.GetLength(0); ++x)
                    {
                        f.Write(",");
                        f.Write(Tiles[x, y]);
                    }
                    f.WriteLine();
                }
            }
            using (var fs = new FileStream(filename + ".png", FileMode.Create))
                Image.SaveAsPng(fs);
        }
        public static Image<Rgba32> ConvertImage(Rgb[,] pixels)
        {
            var img = new Image<Rgba32>(pixels.GetLength(0), pixels.GetLength(1));
            for (int y = 0; y < pixels.GetLength(1); ++y)
                for (int x = 0; x < pixels.GetLength(0); ++x)
                    img[x, y] = pixels[x, y].ToRgba32();
            return img;
        }
    }

    [Flags]
    public enum TileFlags
    {
        None = 0,
        Impassable = 1,
    }

    class Tile
    {
        public readonly string Name;
        public readonly string Sound; // movement sound filename
        public readonly TileFlags Flags;
        public readonly Rgb[] Colors;
        public Tile(string name, TileFlags flags, Rgb[] colors) { Name = name; Flags = flags; Colors = colors; }
        string[] GetFlagsList()
        {
            var flags = (TileFlags[])Enum.GetValues(typeof(TileFlags));
            return flags
                .Where(f => f != TileFlags.None && Flags.HasFlag(f))
                .Select(f => f.ToString())
                .ToArray();
        }
        public string ToFileRepr()
        {
            var pieces = new string[]
            {
                Name, Sound,
                String.Join(",", GetFlagsList()),
            };
            pieces = pieces.Select(s => s ?? "").ToArray();
            return String.Join(":", pieces);
        }
        public static List<Tile> Tileset()
        {
            return new List<Tile>()
            {
                new Tile("water", TileFlags.Impassable, new[] {
                    Rgb.MD_BLUE, Rgb.MD_BLUE, Rgb.BLUE, Rgb.WHITE, Rgb.GREEN, Rgb.BLACK }),
                new Tile("grass", TileFlags.None, new[] {
                    Rgb.MD_RED, Rgb.MD_GREEN, Rgb.MD_GREEN, Rgb.GREEN, Rgb.YELLOW }),
                new Tile("field", TileFlags.None, new[] {
                    Rgb.MD_RED, Rgb.MD_ORANGE, Rgb.BLACK, Rgb.YELLOW, Rgb.YELLOW }),
                new Tile("trees", TileFlags.None, new[] {
                    Rgb.DK_GREEN, Rgb.DK_GREEN, Rgb.DK_YELLOW, Rgb.DK_GRAY }),
                new Tile("mount", TileFlags.Impassable, new[] {
                    Rgb.MD_ORANGE, Rgb.MD_ORANGE, Rgb.BLACK, Rgb.WHITE, Rgb.BLUE_CYAN, Rgb.DK_GREEN }),
            };
        }
    }
}
