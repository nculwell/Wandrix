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

    }

    public class TiledMap
    {
        public const int FILENAME_LENGTH_LIMIT = 28;

        public int Width { get; private set; }
        public int Height { get; private set; }
        public int TileWidth { get; private set; }
        public int TileHeight { get; private set; }
        public IList<TiledTileset> Tilesets { get; private set; }
        public IList<TiledLayer> Layers { get; private set; }

        private TiledMap() { }

        public static TiledMap Parse(string tiledMapFilename)
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
            var objectGroupElements = mapElt.SelectNodes("objectgroup");
            var objectGroups = MapElements(objectGroupElements, e => TiledObjectGroup.Parse(e, tileWidth, tileHeight));
            return new TiledMap()
            {
                Width = width,
                Height = height,
                TileWidth = tileWidth,
                TileHeight = tileHeight,
                Tilesets = tilesets.ToList(),
                Layers = layers.ToList(),
            };
        }

        public void Save(string filename)
        {
            foreach (var ts in Tilesets)
            {
                if (ts.ImageFilename.Length > FILENAME_LENGTH_LIMIT)
                    throw new InvalidDataException(
                        $"Image filename more than {FILENAME_LENGTH_LIMIT} characters: {ts.ImageFilename}");
            }
            CheckCellValueLimit();
            using (var w = new PortableBinaryWriter(filename))
            {
                w.Write(Width);
                w.Write(Height);
                w.Write(TileWidth);
                w.Write(TileHeight);
                w.Write(Tilesets.Count());
                w.Write(Layers.Count());
                foreach (var ts in Tilesets)
                {
                    w.Write(ts.FirstGid);
                    w.Write(FILENAME_LENGTH_LIMIT, ts.TilesetFilenameNewExt);
                }
                foreach (var layer in Layers)
                {
                    for (int r = 0; r < Height; ++r)
                        for (int c = 0; c < Width; ++c)
                            w.Write((short)layer.Cells[r][c]);
                }
            }
            foreach (var ts in Tilesets)
            {
                using (var w = new PortableBinaryWriter(ts.TilesetFilenameNewExt))
                {
                    w.Write(ts.ImageWidth);
                    w.Write(ts.ImageHeight);
                    w.Write(ts.TileCount);
                    w.Write(ts.Columns);
                    w.Write(FILENAME_LENGTH_LIMIT, ts.ImageFilename);
                }
            }
        }

        private int CheckCellValueLimit()
        {
            int[] cellValueLimits = new int[] { SByte.MaxValue, Int16.MaxValue, Int32.MaxValue };
            int cellValueLimitIndex = 0;
            foreach (var layer in Layers)
            {
                for (int r = 0; r < Height; ++r)
                    for (int c = 0; c < Width; ++c)
                        if (layer.Cells[r][c] > Int16.MaxValue)
                            throw new InvalidDataException(
                                $"Cell value too large: {layer.Cells[r][c]} > {Int16.MaxValue}");
            }
            return cellValueLimits[cellValueLimitIndex];
        }

        public class TiledTileset
        {
            public int FirstGid { get; private set; }
            public string TilesetFilename { get; private set; }
            public string TilesetFilenameNewExt
            {
                get { return Path.ChangeExtension(TilesetFilename, ".wts"); }
            }
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

        public class TiledLayer
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
        }

        public class TiledObjectGroup
        {
            public string Name { get; private set; }
            public IList<TiledObject> Objects { get; private set; }
            private TiledObjectGroup() { }
            internal static TiledObjectGroup Parse(XmlElement objGrpElt, int tileWidth, int tileHeight)
            {
                string name = GetStrAttribute(objGrpElt, "name");
                var objectElements = objGrpElt.SelectNodes("object");
                var objects = MapElements(objectElements, e => TiledObject.Parse(e, tileWidth, tileHeight));
                return new TiledObjectGroup() { Name = name, Objects = objects.ToList(), };
            }
        }

        public class TiledObject
        {
            public int Id { get; private set; }
            public string Name { get; private set; }
            public int Gid { get; private set; }
            public int X { get; private set; }
            public int Y { get; private set; }
            public Dictionary<string, string> Properties { get; private set; }
            private TiledObject() { }
            public static TiledObject Parse(XmlElement objElt, int tileWidth, int tileHeight)
            {
                int id = GetIntAttribute(objElt, "id");
                string name = GetStrAttribute(objElt, "name");
                int gid = GetIntAttribute(objElt, "id");
                int x = GetIntAttribute(objElt, "x") / tileWidth;
                int y = GetIntAttribute(objElt, "y") / tileHeight;
                RequireAttributeValue(objElt, "width", tileWidth.ToString());
                RequireAttributeValue(objElt, "height", tileHeight.ToString());
                var properties = new Dictionary<string, string>();
                var propertyElements = objElt.SelectNodes("properties/property");
                foreach (XmlElement propElt in propertyElements)
                {
                    string propName = GetStrAttribute(propElt, "name");
                    string propValue = GetStrAttribute(propElt, "value");
                    properties.Add(propName, propValue);
                }
                return new TiledObject()
                {
                    Id = id,
                    Name = name,
                    Gid = gid,
                    X = x,
                    Y = y,
                    Properties = properties,
                };
            }
        }

        /*
          <object id = "1" name="Pot1" gid="28" x="1920" y="1120" width="32" height="32">
            <properties>
              <property name = "Script" value="KillPlayer()"/>
            </properties>
        */

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

        private static int ParseInt(string s)
        {
            int i;
            Int32.Parse(s);
            if (!Int32.TryParse(s, out i))
                throw new FormatException("Invalid integer: '" + s + "'");
            return i;
        }

        private static void WriteIntBigEndian(BinaryWriter writer, int intToWrite)
        {
            byte[] bytes = BitConverter.GetBytes(intToWrite);
            if (BitConverter.IsLittleEndian)
                bytes = bytes.Reverse().ToArray();
            writer.Write(bytes);
        }

    }
}
