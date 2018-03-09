using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using SixLabors.ImageSharp;

namespace Generator
{
    struct Rgb
    {
        public readonly byte R, G, B;
        public uint RGB { get { return (uint)R << 16 | (uint)G << 8 | B; } }
        public Rgb(uint rgb)
            : this((byte)((rgb & 0xFF0000) >> 16), (byte)((rgb & 0xFF00) >> 8), (byte)(rgb & 0xFF)) { }
        public Rgb(byte r, byte g, byte b) { R = r; G = g; B = b; }
        private Rgb(byte[] rgb) : this(rgb[0], rgb[1], rgb[2]) { }
        public Rgba32 ToRgba32() { return new Rgba32(R, G, B); }
        private static Rgb Combine(Rgb x, Rgb y)
        {
            int r = x.R + y.R;
            int g = x.G + y.G;
            int b = x.B + y.B;
            if (r > 0xFF | b > 0xFF | g > 0xFF)
                throw new ArgumentOutOfRangeException("At least one color component is out of range.");
            return new Rgb((byte)r, (byte)g, (byte)b);
        }
        private Rgb Reduce(float factor)
        {
            byte[] rgb = new[] { R, G, B }.Select(x => (byte)Math.Floor(x / factor)).ToArray();
            return new Rgb(rgb);
        }
        public byte To8Bit()
        {
            int r = ChannelTo2Bit(R) << 4;
            int g = ChannelTo2Bit(G) << 2;
            int b = ChannelTo2Bit(B) << 0;
            int combined = r | g | b;
            return (byte)combined;
        }
        private byte ChannelTo2Bit(byte c)
        {
            switch (c)
            {
                case MAX: return 3;
                case MID: return 2;
                case DRK: return 1;
                case 0: return 0;
                default:
                    throw new ArgumentOutOfRangeException("Invalid channel value for 8-bit color: " + c);
            }
        }
        private const byte MAX = 0xFF, MID = 0x80, DRK = 0x40, DIM = 0x20;
        public static readonly Rgb WHITE = new Rgb(MAX, MAX, MAX);
        public static readonly Rgb MD_GRAY = new Rgb(MID, MID, MID);
        public static readonly Rgb DK_GRAY = new Rgb(DRK, DRK, DRK);
        public static readonly Rgb BLACK = new Rgb(0, 0, 0);
        public static readonly Rgb RED = new Rgb(MAX, 0, 0);
        public static readonly Rgb MD_RED = new Rgb(MID, 0, 0);
        public static readonly Rgb DK_RED = new Rgb(DRK, 0, 0);
        public static readonly Rgb GREEN = new Rgb(0, MAX, 0);
        public static readonly Rgb MD_GREEN = new Rgb(0, MID, 0);
        public static readonly Rgb DK_GREEN = new Rgb(0, DRK, 0);
        public static readonly Rgb BLUE = new Rgb(0, 0, MAX);
        public static readonly Rgb MD_BLUE = new Rgb(0, 0, MID);
        public static readonly Rgb DK_BLUE = new Rgb(0, 0, DRK);
        public static readonly Rgb YELLOW = new Rgb(MAX, MAX, 0);
        public static readonly Rgb MD_YELLOW = new Rgb(MID, MID, 0);
        public static readonly Rgb DK_YELLOW = new Rgb(DRK, DRK, 0);
        public static readonly Rgb CYAN = new Rgb(0, MAX, MAX);
        public static readonly Rgb MD_CYAN = new Rgb(0, MID, MID);
        public static readonly Rgb DK_CYAN = new Rgb(0, DRK, DRK);
        public static readonly Rgb MAGENTA = new Rgb(MAX, 0, MAX);
        public static readonly Rgb MD_MAGENTA = new Rgb(MID, 0, MID);
        public static readonly Rgb DK_MAGENTA = new Rgb(DRK, 0, DRK);
        public static readonly Rgb ORANGE = new Rgb(MAX, MID, 0);
        public static readonly Rgb MD_ORANGE = new Rgb(MID, DRK, 0);
        //public static readonly Rgb DK_ORANGE = new Rgb(DRK, DIM, 0);
        public static readonly Rgb BLUE_CYAN = new Rgb(0, MID, MAX);
        public static readonly Rgb MD_BLUE_CYAN = new Rgb(0, DRK, MID);
        //public static readonly Rgb DK_BLUE_CYAN = new Rgb(0, DIM, DRK);
    }
}
