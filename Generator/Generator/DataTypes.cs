using System;
using System.Collections.Generic;
using System.Text;

namespace Generator
{

    struct Size
    {
        public readonly int W, H;
        public Size(int w, int h) { W = w; H = h; }
        public static Size operator *(Size x, Size y)
        {
            return new Size(x.W * y.W, x.H * y.H);
        }
    }

    struct Point
    {
        public readonly int X, Y;
        public Point(int x, int y) { X = x; Y = y; }
    }

    struct Box
    {
        public readonly int X, Y, W, H;
        public Box(int x, int y, int w, int h) { X = x; Y = y; W = w; H = h; }
        public Box(Point xy, Size wh) : this(xy.X, xy.Y, wh.W, wh.H) { }
        public Box(object[,] array) : this(0, 0, array.GetLength(0), array.GetLength(1)) { }
        public Box(int[,] array) : this(0, 0, array.GetLength(0), array.GetLength(1)) { }
        public bool Contains(Point p)
        {
            return !(p.X < X || p.Y < Y || p.X >= X + W || p.Y >= Y + H);
        }
    }

}
