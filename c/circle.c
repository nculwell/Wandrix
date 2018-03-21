
#include <SDL.h>

static SDL_Surface* CreateSurface(int width, int height)
{
  Uint32 rmask = SDL_SwapBE32(0xff000000),
         gmask = SDL_SwapBE32(0x00ff0000),
         bmask = SDL_SwapBE32(0x0000ff00),
         amask = SDL_SwapBE32(0x000000ff);
  SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height, 32, rmask, gmask, bmask, amask);
  if (!surface)
    fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
  return surface;
}

SDL_Surface* CreateQuarterCircle(int radius, Uint32 colorRGBA)
{
  // Draw a quarter circle using the midpoint algorithm.
  SDL_Surface* surface = CreateSurface(radius, radius);
  if (!surface)
  {
    fprintf(stderr, "Unable to create surface. %s\n", SDL_GetError());
    return 0;
  }
  SDL_FillRect(surface, 0, 0);
  Uint32 fillColor = SDL_SwapBE32(colorRGBA);
#define SET_PIXEL(X,Y) (((Uint32*)surface->pixels)[(Y) * radius + (X)] = fillColor)
  int x = radius - 1;
  int y = 0;
  int dx = 1;
  int dy = 1;
  int error = dx - (radius << 1);
  while (x >= y)
  {
    SDL_Rect rect1 = { 0, y, x, 1 };
    SDL_Rect rect2 = { 0, x, y, 1 };
    SDL_FillRect(surface, &rect1, fillColor);
    SDL_FillRect(surface, &rect2, fillColor);
    if (error <= 0)
    {
      ++y;
      error += dy;
      dy += 2;
    }
    if (error > 0)
    {
      --x;
      error += dx - (radius << 1);
      dx += 2;
    }
  }
  return surface;
}


