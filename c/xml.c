/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include "wandrix.h"

typedef struct XmlAttribute {
  const char* name;
  const char* value;
  struct XmlAttribute* next;
} XmlAttribute;

typedef struct XmlElement {
  const char* name;
  struct XmlAttribute* attribute;
  struct XmlElement* child;
  struct XmlElement* sibling;
  //struct XmlElement* parent;
} XmlElement;

struct XmlParser {
  char* xml;
  int pos, len;
};

XmlElement* ParseElement(XmlParser* p);

XmlElement* XmlParse(const char* filename)
{
  XmlParser parser;
  if (!ReadBinFile(filename, &parser.xml, &parser.len)) return 0;
  return ParseElement(&parser);
}

XmlElement* ParseElement(XmlParser* p)
{
  return 0;
}

