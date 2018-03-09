using System;
using System.IO;

namespace Generator
{
    class Program
    {
        static Size MapSize = new Size(16, 16);
        static Size TileSize = new Size(64, 64);
        static void Main(string[] args)
        {
            string dir = (args.Length > 0 ? args[0] : ".");
            Directory.SetCurrentDirectory(dir);
            var map = Generator.NewMap(MapSize, TileSize);
            map.Save("map");
        }
    }
}
