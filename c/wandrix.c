/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

const char* WINDOW_NAME = "Wandrix";
const int SCREEN_W = 800;
const int SCREEN_H = 600;
const int FULLSCREEN = 0;
const int VSYNC = 1;
const Uint32 LOGIC_FRAMES_PER_SEC = 16; // fixed rate
const Uint32 RENDER_FRAMES_PER_SEC = 32; // rate cap

struct Coords { int x, y; };
struct Size { int w, h; };
struct CharBase {
  const char* name; struct Coords pos, mov; int hpCur, hpMax;
  const char* imgFilename; SDL_Surface* img; SDL_Texture* tex;
};
struct Player {
  struct CharBase c;
};
struct Npc {
  struct CharBase c;
};

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Surface* screen;
SDL_Texture* map;
struct Coords center;
struct Size maxTextureSize;
struct Size mapImageSize;
int quitting = 0;
struct Player player = {
  { "Player", { 0, 0 }, { 0, 0 }, 0, 0,
  "ckclose.png", NULL, NULL }
};
struct Npc npcs[128];

void atExitHandler()
{
  //SDL_DestroyTexture(bitmapTex);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

int init()
{
  if(SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
    return 0;
  }
  int imgFlags = IMG_INIT_PNG;
  int imgInitResult = IMG_Init(imgFlags) & imgFlags;
  if(!imgInitResult)
  {
    fprintf(stderr, "SDL_image init failed: %s\n", IMG_GetError() );
    return 0;
  }
  Uint32 windowFlags = (FULLSCREEN * SDL_WINDOW_FULLSCREEN_DESKTOP);
  int wPos = SDL_WINDOWPOS_CENTERED;
  window = SDL_CreateWindow(WINDOW_NAME, wPos, wPos, SCREEN_W, SCREEN_H, windowFlags);
  if (!window)
  {
    fprintf(stderr, "Create window failed: %s\n", SDL_GetError());
    return 0;
  }
  renderer = SDL_CreateRenderer(window, -1,
      SDL_RENDERER_ACCELERATED | (VSYNC * SDL_RENDERER_PRESENTVSYNC));
  if (!renderer)
  {
    fprintf(stderr, "Create renderer failed: %s\n", SDL_GetError());
    return 0;
  }
  SDL_RendererInfo rendererInfo;
  if (0 > SDL_GetRendererInfo(renderer, &rendererInfo))
  {
    fprintf(stderr, "Get renderer info failed: %s\n", SDL_GetError());
    return 0;
  }
  maxTextureSize.w = rendererInfo.max_texture_width;
  maxTextureSize.h = rendererInfo.max_texture_height;
  printf("Max texture size: %dx%d\n", maxTextureSize.w, maxTextureSize.h);
  screen = SDL_GetWindowSurface(window);
  center.x = screen->w / 2;
  center.y = screen->h / 2;
  atexit(atExitHandler);
  return 1;
}

int loadImage(const char* path, struct Size* size, SDL_Surface** surface, SDL_Texture** texture)
{
  assert(path);
  assert(surface);
  SDL_Surface* loadedSurface = IMG_Load(path);
  if (!loadedSurface)
  {
    fprintf(stderr, "Unable to load image '%s': %s\n", path, IMG_GetError());
    return 0;
  }
  *surface = SDL_ConvertSurface(loadedSurface, screen->format, 0);
  if (!*surface)
  {
    fprintf(stderr, "Unable to optimize image '%s': %s\n", path, SDL_GetError());
    goto loadError;
  }
  if (size != NULL && (loadedSurface->w != size->w || loadedSurface->h != size->h))
  {
    *surface = SDL_CreateRGBSurfaceWithFormat(
        0, size->w, size->h, screen->format->BitsPerPixel, screen->format->format);
    if (!*surface)
    {
      fprintf(stderr, "Unable to create surface '%s': %s\n", path, SDL_GetError());
      goto loadError;
    }
    if (!SDL_BlitScaled(loadedSurface, NULL, *surface, NULL))
    {
      fprintf(stderr, "Unable to scale '%s': %s\n", path, SDL_GetError());
      goto loadError;
    }
  }
  SDL_FreeSurface(loadedSurface);
  loadedSurface = NULL;
  if (texture)
  {
    *texture = SDL_CreateTextureFromSurface(renderer, *surface);
    if (!*texture)
    {
      fprintf(stderr, "Unable to create texture for image '%s': %s\n", path, SDL_GetError());
      goto loadError;
    }
  }
  return 1;
loadError:
  if (loadedSurface) SDL_FreeSurface(loadedSurface);
  if (*surface) SDL_FreeSurface(*surface);
  return 0;
}

/* const char* name; int x, y; int hpCur, hpMax;
   const char* imgFilename; SDL_Surface* img; SDL_Texture* tex; */

int loadCharImage(struct CharBase* c)
{
  struct Size s = { 64, 64 };
  return loadImage(c->imgFilename, &s, &c->img, &c->tex);
}

int loadAssets()
{
  SDL_Surface* mapSurface;
  if (!loadImage("map.png", NULL, &mapSurface, &map))
    return 0;
  mapImageSize.w = mapSurface->w;
  mapImageSize.h = mapSurface->h;
  SDL_FreeSurface(mapSurface);
  if (!loadCharImage(&player.c))
  {
    fprintf(stderr, "Unable to load player image.\n");
    return 0;
  }
  return 1;
}

struct Coords scanMoveKeys()
{
  struct Coords move = {0,0};
  const Uint8* currentKeyStates = SDL_GetKeyboardState( NULL );
  if (currentKeyStates[SDL_SCANCODE_UP])
    move.y -= 1;
  if (currentKeyStates[SDL_SCANCODE_DOWN])
    move.y += 1;
  if (currentKeyStates[SDL_SCANCODE_LEFT])
    move.x -= 1;
  if (currentKeyStates[SDL_SCANCODE_RIGHT])
    move.x += 1;
  //if (currentKeyStates[SDL_SCANCODE_Q])
  //  quitting = 1;
  return move;
}

/*
function scanMoveKeys()
  local move = {x=0, y=0}
  local m = glo.moveKeys
  glo.moveKeys = {}
  if love.keyboard.isDown("left") or m["left"] then move.x = move.x - 1 end
  if love.keyboard.isDown("right") or m["right"] then move.x = move.x + 1 end
  if love.keyboard.isDown("up") or m["up"] then move.y = move.y - 1 end
  if love.keyboard.isDown("down") or m["down"] then move.y = move.y + 1 end
  move.x = move.x / MOVES_PER_TILE
  move.y = move.y / MOVES_PER_TILE
  return move
end
*/

void updateLogic()
{
  // Apply previous move.
  player.c.pos.x += player.c.mov.x * 8;
  player.c.pos.y += player.c.mov.y * 8;
  // Get next move. (We need it now to interpolate.)
  player.c.mov = scanMoveKeys();
  // TODO: Cancel move if invalid.
}

void updateDisplay(Uint32 phase)
{
  // TODO: Interpolate object positions.
}

int drawTexture(SDL_Rect* screenRect, SDL_Texture* texture, SDL_Rect* textureRect)
{
  SDL_Rect intersectRect;
  int intersects =
    (SDL_TRUE == SDL_IntersectRect(screenRect, textureRect, &intersectRect));
  if (intersects) {
    SDL_Rect screenDestRect = {
      intersectRect.x - screenRect->x, intersectRect.y - screenRect->y,
      intersectRect.w, intersectRect.h };
    SDL_RenderCopy(renderer, texture, &intersectRect, &screenDestRect);
  }
  return intersects;
}

void draw()
{
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderClear(renderer);
  SDL_Rect screenRect = {
    player.c.pos.x - center.x, player.c.pos.y - center.y,
    screen->w, screen->h };
  SDL_Rect wholeMapRect = { 0, 0, mapImageSize.w, mapImageSize.h };
  drawTexture(&screenRect, map, &wholeMapRect);
  SDL_RenderPresent(renderer);
}

void handleKeypress(SDL_Event* e)
{
  switch (e->key.keysym.sym) {
    // case SDLK_UP:
    case SDLK_q: quitting = 1; break;
  }
}

void pollEvents()
{
  SDL_Event e;
  if (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      quitting = 1;
    }
    if (e.type == SDL_KEYDOWN) {
      handleKeypress(&e);
    }
  }
}

int mainLoop()
{
  Uint32 startTime = SDL_GetTicks(),
         nextLogicFrameTime = 0,
         nextRenderFrame = 0,
         logicFrameTimeMs = 1000 / LOGIC_FRAMES_PER_SEC,
         renderFrameTimeMs = 1000 / RENDER_FRAMES_PER_SEC;
  while (!quitting)
  {
    // TODO: Reset frame times once per second to prevent rounding
    // error from accumulating.
    Uint32 time = SDL_GetTicks() - startTime;
    pollEvents();
    if (nextLogicFrameTime > time)
      SDL_Delay(0); // Be a little nice with the CPU when ahead of schedule.
    while (nextLogicFrameTime < time) {
      nextLogicFrameTime += logicFrameTimeMs;
      updateLogic();
    }
    //int skipDraw = 0;
    int skipDraw = 1;
    while (nextRenderFrame < time) {
      nextRenderFrame += renderFrameTimeMs;
      skipDraw = 0;
    }
    if (!skipDraw) {
      Uint32 phase = 1000 - (nextLogicFrameTime - time) * 1000 / LOGIC_FRAMES_PER_SEC;
      updateLogic(phase);
      draw();
    }
  }
  return 1;
}

int main(int argc, char** argv)
{
  printf("STARTED\n");
  int success = init() && loadAssets() && mainLoop();
  printf("FINISHED\n");
  return !success;
}

