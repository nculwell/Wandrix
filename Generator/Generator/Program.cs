using System;

namespace Generator
{
    class Program
    {
        static Size MapSize = new Size(100, 100);
        static Size TileSize = new Size(100, 100);
        static void Main(string[] args)
        {
            var map = Generator.NewMap(MapSize, TileSize);
            map.Save("map");
        }
    }
}
