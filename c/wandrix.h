/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

struct Coords Coords_Scale(int scalar, struct Coords s);
struct Coords Coords_Add(struct Coords a, struct Coords b);
struct SDL_Rect Rect_Combine(struct Coords c, struct Size s);
SDL_Rect CharBase_GetRect(struct CharBase* c);
struct Size CharBase_GetSize(struct CharBase* c);
const char* Rect_ToString(SDL_Rect* r, char* buf);

