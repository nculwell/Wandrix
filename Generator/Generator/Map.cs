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
        public Rgb[,] Image;
        public void Save(string filename)
        {
            using (var fs = new FileStream(filename + ".dat", FileMode.Create))
            using (var f = new BinaryWriter(fs))
            {
                int[] header = new[] {
                    Tiles.GetLength(0), Tiles.GetLength(1),
                    //Image.GetLength(0), Image.GetLength(1),
                };
                foreach (int h in header)
                    f.Write(IPAddress.HostToNetworkOrder(h));
                foreach (short tileId in Tiles)
                    f.Write(IPAddress.HostToNetworkOrder(tileId));
                //foreach (Rgb pixel in Image)
                //{
                //    f.Write(pixel.To8Bit());
                //    //f.Write(pixel.R);
                //    //f.Write(pixel.G);
                //    //f.Write(pixel.B);
                //}
            }
            var img = new Image<Rgba32>(Image.GetLength(0), Image.GetLength(1));
            for (int y = 0; y < Image.GetLength(1); ++y)
            {
                for (int x = 0; x < Image.GetLength(0); ++x)
                {
                    img[x, y] = Image[x, y].ToRgba32();
                }
            }
            using (var fs = new FileStream(filename + ".png", FileMode.OpenOrCreate))
                img.SaveAsPng(fs);
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
