/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#define FILENAME_BUFSIZE 32

struct Coords { int x, y; };
struct Size { int w, h; };
struct Image { 
  const char path[FILENAME_BUFSIZE]; SDL_Surface* sfc; SDL_Texture* tex;
};
struct CharBase {
  const char* name; struct Image img; struct Coords pos, mov; int hpCur, hpMax;
};
struct Player {
  struct CharBase c;
};
struct Npc {
  int id; // set to 0 if this slot is empty
  struct CharBase c;
};

struct TextFile {
  int nLines;
  char** lines;
};
struct IntGrid {
  Uint16 rows, cols;
  Sint16* cells;
};


#define Rect_UNPACK(SDL_RECT_PTR) \
  ((SDL_RECT_PTR)->x), ((SDL_RECT_PTR)->y), ((SDL_RECT_PTR)->w), ((SDL_RECT_PTR)->h)

struct Coords Coords_Scale(int scalar, struct Coords s);
struct Coords Coords_Add(struct Coords a, struct Coords b);
struct SDL_Rect Rect_Combine(struct Coords c, struct Size s);
SDL_Rect CharBase_GetRect(struct CharBase* c);
struct Size CharBase_GetSize(struct CharBase* c);
const char* Rect_ToString(SDL_Rect* r, char* buf);

struct TextFile* ReadTextFile(const char* filename);
void FreeTextFile(struct TextFile* lines);
struct IntGrid* ReadGridFile(const char* filename, int nRows, int nCols);
void FreeIntGrid(struct IntGrid* grid);

Sint32 ParseInt32(const char* str);
Sint16 ParseInt16(const char* str);

