/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include <stdio.h>

typedef struct XmlElement {
  const char* name;
  struct XmlElement* child;
  struct XmlElement* sibling;
  //struct XmlElement* parent;
} XmlElement;

struct XmlParser {
  char* xml;
  int pos;
};

XmlElement* XmlParse(const char* filename)
{
  FILE* f = fopen(filename, "rb");
}

XmlElement* ParseElement(XmlParser* p)
{
}

