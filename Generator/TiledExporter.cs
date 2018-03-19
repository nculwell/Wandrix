using System;
using System.Collections.Generic;
using System.Text;
using System.Linq;
using System.Net;
using System.Xml;
using System.IO;

namespace Generator
{
    class TiledExporter
    {

        public TiledExporter() { }

        public void Export(string tiledMapFilename, string destBaseFilename)
        {
            var tilemapDoc = new XmlDocument();
            tilemapDoc.Load(tiledMapFilename);
            var mapElt = tilemapDoc.DocumentElement;
            RequireAttributeValue(mapElt, "orientation", "orthogonal");
            // TODO: Support other render orders.
            RequireAttributeValue(mapElt, "renderorder", "right-down");
            int width = GetIntAttribute(mapElt, "width");
            int height = GetIntAttribute(mapElt, "height");
            int tileWidth = GetIntAttribute(mapElt, "tilewidth");
            int tileHeight = GetIntAttribute(mapElt, "tileheight");
            var tilesetElements = mapElt.SelectNodes("tileset");
            var tilesets = MapElements(tilesetElements, e => TiledTileset.Parse(e, tileWidth, tileHeight));
            var layerElements = mapElt.SelectNodes("layer");
            var layers = MapElements(layerElements, e => TiledLayer.Parse(e, width, height));
        }

        class TiledTileset
        {
            public int FirstGid { get; private set; }
            public string TilesetFilename { get; private set; }
            public string ImageFilename { get; private set; }
            public int ImageWidth { get; private set; }
            public int ImageHeight { get; private set; }
            public int TileCount { get; private set; }
            public int Columns { get; private set; }
            private TiledTileset() { }
            public static TiledTileset Parse(XmlElement tilesetElement, int expectedTileWidth, int expectedTileHeight)
            {
                if (tilesetElement.Name != "tileset")
                    throw new ArgumentException("Element is not a tileset: " + tilesetElement.Name);
                string source = GetStrAttribute(tilesetElement, "source");
                int firstGid = GetIntAttribute(tilesetElement, "firstgid");
                return ParseDoc(source, firstGid, expectedTileWidth, expectedTileHeight);
            }
            private static TiledTileset ParseDoc(string source, int firstGid, int expectedTileWidth, int expectedTileHeight)
            {
                var tilesetDoc = new XmlDocument();
                tilesetDoc.Load(source);
                var tilesetRoot = tilesetDoc.DocumentElement;
                RequireAttributeValue(tilesetRoot, "tilewidth", expectedTileWidth.ToString());
                RequireAttributeValue(tilesetRoot, "tileheight", expectedTileHeight.ToString());
                int tileCount = GetIntAttribute(tilesetRoot, "tilecount");
                int columns = GetIntAttribute(tilesetRoot, "columns");
                var imageElt = (XmlElement)tilesetRoot.SelectSingleNode("image");
                string imageSource = imageElt.Attributes["source"].Value;
                int imageWidth = GetIntAttribute(imageElt, "width");
                int imageHeight = GetIntAttribute(imageElt, "height");
                return new TiledTileset()
                {
                    FirstGid = firstGid,
                    TilesetFilename = source,
                    TileCount = tileCount,
                    Columns = columns,
                    ImageFilename = imageSource,
                    ImageWidth = imageWidth,
                    ImageHeight = imageHeight,
                };
            }
            public void SaveText(System.IO.TextWriter w)
            {
                w.WriteLine("TiledTileset");
                w.WriteLine(DateTime.UtcNow.ToString("o"));
                w.WriteLine("Filename=" + TilesetFilename);
                w.WriteLine("FirstGid=" + FirstGid);
                w.WriteLine("TileCount=" + TileCount);
                w.WriteLine("Columns=" + Columns);
                w.WriteLine("ImageFilename=" + ImageFilename);
                w.WriteLine("ImageWidth=" + ImageWidth);
                w.WriteLine("ImageHeight=" + ImageHeight);
                w.WriteLine("END");
            }
        }

        /*
        <tileset name="sharm-tiny-16-basic-basictiles32" tilewidth="32" tileheight="32" tilecount="120" columns="8">
        <image source="sharm-tiny-16-basic-basictiles32.png" width="256" height="480"/>
         */

        class TiledLayer
        {
            public string Name { get; private set; }
            public int[][] Cells { get; private set; }
            private TiledLayer() { }
            public static TiledLayer Parse(XmlElement layerElement, int expectedWidth, int expectedHeight)
            {
                if (layerElement.Name != "layer")
                    throw new ArgumentException("Element is not a layer: " + layerElement.Name);
                string name = GetStrAttribute(layerElement, "name");
                RequireAttributeValue(layerElement, "width", expectedWidth.ToString());
                RequireAttributeValue(layerElement, "height", expectedHeight.ToString());
                var dataElement = (XmlElement)layerElement.SelectSingleNode("data");
                RequireAttributeValue(dataElement, "encoding", "csv");
                string data = dataElement.InnerText.Trim().Replace("\r", "");
                var dataLines = data.Split('\n');
                var dataCells = dataLines
                    .Select(x =>
                        x.Split(',')
                        .Select(Int32.Parse)
                        .ToArray())
                    .ToArray();
                return new TiledLayer() { Name = name, Cells = dataCells, };
            }
            public void SaveText(TextWriter w)
            {
                w.WriteLine("TiledLayer");
                w.WriteLine(DateTime.UtcNow.ToString("o"));
                w.WriteLine("Name=" + Name);
                w.WriteLine("END");
            }
            public void SaveData(BinaryWriter w, int rows, int cols)
            {
                for (int r = 0; r < rows; ++r)
                    for (int c = 0; c < cols; ++c)
                        WriteIntBigEndian(w, Cells[r][c]);
            }
            private void WriteIntBigEndian(BinaryWriter writer, int intToWrite)
            {
                byte[] bytes = BitConverter.GetBytes(intToWrite);
                if (BitConverter.IsLittleEndian)
                    bytes = bytes.Reverse().ToArray();
                writer.Write(bytes);
            }
        }

        private static IEnumerable<Tout> MapElements<Tout>(XmlNodeList elements, Func<XmlElement, Tout> function)
        {
            foreach (XmlElement elt in elements)
            {
                yield return function(elt);
            }
        }

        private static void RequireAttributeValue(XmlElement elt, string attrName, string desiredAttrValue)
        {
            string actualAttrValue = elt.Attributes[attrName]?.Value;
            if (actualAttrValue == null)
                throw new Exception($"Attribute missing from element {elt.Name}: {attrName}");
            if (actualAttrValue != desiredAttrValue)
                throw new Exception($"Invalid attribute value: expected '{desiredAttrValue}' but found '{actualAttrValue}'.");
        }

        private static string GetStrAttribute(XmlElement elt, string attrName)
        {
            return elt.Attributes[attrName].Value;
        }

        private static int GetIntAttribute(XmlElement elt, string attrName)
        {
            return ParseInt(elt.Attributes[attrName].Value);
        }

        public static int ParseInt(string s)
        {
            int i;
            Int32.Parse(s);
            if (!Int32.TryParse(s, out i))
                throw new FormatException("Invalid integer: '" + s + "'");
            return i;
        }

    }
}
