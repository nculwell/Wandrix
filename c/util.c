/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct Coords { int x, y; };
struct Size { int w, h; };
struct Image { 
  const char path[32]; SDL_Surface* sfc; SDL_Texture* tex;
};
struct CharBase {
  const char* name; struct Image img; struct Coords pos, mov, phs; int hpCur, hpMax;
};
struct Player {
  struct CharBase c;
};
struct Npc {
  int id; // set to 0 if this slot is empty
  struct CharBase c;
};

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

