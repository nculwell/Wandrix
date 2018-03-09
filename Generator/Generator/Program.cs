using System;
using System.IO;

namespace Generator
{
    class Program
    {
        static Size MapSize = new Size(10, 10);
        static Size TileSize = new Size(100, 100);
        static void Main(string[] args)
        {
            string dir = (args.Length > 0 ? args[0] : ".");
            Directory.SetCurrentDirectory(dir);
            var map = Generator.NewMap(MapSize, TileSize);
            map.Save("map");
        }
    }
}
