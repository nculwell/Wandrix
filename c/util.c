/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include "wandrix.h"
#include <SDL_image.h>

extern SDL_Renderer* renderer;
extern SDL_Surface* screen;

struct Coords Coords_Scale(int scalar, struct Coords s)
{
  s.x *= scalar;
  s.y *= scalar;
  return s;
}

struct Coords Coords_Add(struct Coords a, struct Coords b)
{
  struct Coords sum = { a.x + b.x, a.y + b.y };
  return sum;
}

struct SDL_Rect Rect_Combine(struct Coords c, struct Size s)
{
  SDL_Rect r = { c.x, c.y, s.w, s.h };
  return r;
}

SDL_Rect CharBase_GetRect(struct CharBase* c)
{
  SDL_Rect r = { c->pos.x, c->pos.y, c->img.sfc->w, c->img.sfc->h };
  return r;
}

struct Size CharBase_GetSize(struct CharBase* c)
{
  struct Size s = { c->img.sfc->w, c->img.sfc->h };
  return s;
}

const char* Rect_ToString(SDL_Rect* r, char* buf)
{
  sprintf(buf, "(x=%d,y=%d,w=%d,h=%d)", r->x, r->y, r->w, r->h);
  return buf;
}

void* MallocOrDie(size_t size)
{
  void* mem = calloc(1, size);
  if (!mem)
  {
    fprintf(stderr, "Out of memory.\n");
    exit(-1);
  }
  return mem;
}

int ReadBinFile(const char* filename, char** filePtr, long* fileLen)
{
  *filePtr = 0;
  *fileLen = 0;
  FILE* f = fopen(filename, "rb");
  if (!f)
  {
    fprintf(stderr, "Error opening file '%s': %s\n", filename, strerror(errno));
    return 0;
  }
  fseek(f, 0L, SEEK_END);
  long len = ftell(f);
  fseek(f, 0L, SEEK_SET);
  char* buf = (char*)MallocOrDie(len);
  size_t toRead = len;
  while (0 != (toRead -= fread(buf, 1, toRead, f)))
  {
    if (ferror(f))
    {
      fprintf(stderr, "Error reading file '%s': %s\n", filename, strerror(errno));
      fclose(f);
      free(buf);
      return 0;
    }
  }
  fclose(f);
  *filePtr = buf;
  *fileLen = len;
  return 1;
}

struct TextFile* ReadTextFile(const char* filename)
{
  char* buf;
  long len;
  if (!ReadBinFile(filename, &buf, &len)) return 0;
  int nLines = 0;
  for (int i = 0; i < len; ++i)
    if (buf[i] == '\n')
      ++nLines;
  char** lines = (char**)MallocOrDie((nLines + 1) * sizeof(char*));
  lines[0] = buf;
  int ln = 0;
  for (int i=0; i < len; ++i)
  {
    if (buf[i] == '\n')
    {
      lines[++ln] = &buf[i + 1];
    }
    if (buf[i] == '\r' || buf[i] == '\n')
    {
      buf[i] = 0;
    }
  }
  lines[nLines] = 0;
  struct TextFile* textFile = MallocOrDie(sizeof(struct TextFile));
  textFile->nLines = nLines;
  textFile->lines = lines;
  return textFile;
}

void FreeTextFile(struct TextFile* textFile)
{
  free(textFile->lines[0]);
  free(textFile->lines);
  free(textFile);
}

struct IntGrid* ReadGridFile(const char* filename, int nRows, int nCols)
{
  struct TextFile* file = ReadTextFile(filename);
  if (!file) return 0;
  struct IntGrid* grid = MallocOrDie(sizeof(struct IntGrid));
  grid->rows = nRows;
  grid->cols = nCols;
  grid->cells = MallocOrDie(nRows * nCols * sizeof(*grid->cells));
  for (int r=0; r < nRows; ++r)
  {
    char* p = file->lines[r];
    assert(p);
    char* e;
    for (int c=0; c < nCols; ++c)
    {
      long n = strtol(p, &e, 10);
      if (c < nCols-1)
        assert(*e == ',');
      else
        assert(*e == '\0');
      p = e + 1;
      grid->cells[r * nCols + c] = n;
    }
  }
  return grid;
}

void FreeIntGrid(struct IntGrid* grid)
{
  free(grid->cells);
  free(grid);
}

long ParseLong(const char* str)
{
  char* end;
  long n = strtol(str, &end, 10);
  if (end == str || *end != '\0')
  {
    fprintf(stderr, "Unable to parse int from string: '%s'\n", str);
    return LONG_MIN;
  }
  return n;
}

Sint32 ParseInt32(const char* str)
{
  long n = ParseLong(str);
  if (n == LONG_MIN)
    return SDL_MIN_SINT32;
  if (n <= SDL_MIN_SINT32 || n > SDL_MAX_SINT32)
  {
    fprintf(stderr, "Number out of range: '%s'\n", str);
    return SDL_MIN_SINT32;
  }
  return (Sint32)n;
}

Sint16 ParseInt16(const char* str)
{
  long n = ParseLong(str);
  if (n == LONG_MIN)
    return SDL_MIN_SINT16;
  if (n <= SDL_MIN_SINT16 || n > SDL_MAX_SINT16)
  {
    fprintf(stderr, "Number out of range: '%s'\n", str);
    return SDL_MIN_SINT16;
  }
  return (Sint16)n;
}

int LoadImage(struct Image* img, int createTexture)
{
  assert(img);
  assert(img->path);
  SDL_Surface* loadedSurface = IMG_Load(img->path);
  if (!loadedSurface)
  {
    fprintf(stderr, "Unable to load image '%s': %s\n", img->path, IMG_GetError());
    return 0;
  }
  img->sfc = loadedSurface;
  if (createTexture)
  {
    img->tex = SDL_CreateTextureFromSurface(renderer, img->sfc);
    if (!img->tex)
    {
      fprintf(stderr, "Unable to create texture for image '%s': %s\n", img->path, SDL_GetError());
      return 0;
    }
  }
  return 1;
}

int InitImage()
{
  int imgFlags = IMG_INIT_PNG;
  int imgInitResult = IMG_Init(imgFlags) & imgFlags;
  if(!imgInitResult)
  {
    fprintf(stderr, "SDL_image init failed: %s\n", IMG_GetError() );
    return 0;
  }
  return 1;
}

int USqrt(unsigned n)
{
  // Find the magnitude of n.
  unsigned shiftBits = 2;
  unsigned shifted = n >> shiftBits;
  while (shifted > 0)
  {
    shiftBits += 2;
    shifted >>= 2;
  }
  shiftBits -= 2;
  // Find digits of result.
  unsigned result = 0;
  do {
    result <<= 1;
    unsigned candidateResult = result + 1;
    unsigned candidateResultSquared = candidateResult * candidateResult;
    if (candidateResultSquared == n >> shiftBits)
      result = candidateResult;
    shiftBits -= 2;
  } while (shiftBits >= 0);
  return result;
}

int SSqrt(int n)
{
  assert(n >= 0);
  return (int)USqrt((unsigned)n);
}

