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
            const int operation = 1;

            string dir = (args.Length > 0 ? args[0] : ".");
            Directory.SetCurrentDirectory(dir);

            if (operation == 0)
            {
                var map = Generator.NewMap(MapSize, TileSize);
                map.Save("map");
            }
            else if (operation == 1)
            {
                var map = TiledMap.Parse("map.tmx");
                map.Save("testmap");
            }
            else throw new InvalidOperationException("Invalid operation code.");
        }
    }
}
