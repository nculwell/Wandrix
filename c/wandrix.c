/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

const char* WINDOW_NAME = "Wandrix";
const int SCREEN_W = 800;
const int SCREEN_H = 600;
const Uint32 LOGIC_FRAMES_PER_SEC = 16; // fixed rate
const Uint32 RENDER_FRAMES_PER_SEC = 32; // rate cap

struct Coords { int x, y; };
struct Size { int w, h; };

struct Coords Coords(int x, int y)
{
  struct Coords c;
  c.x = x;
  c.y = y;
  return c;
}

struct Size Size(int w, int h)
{
  struct Size s;
  s.w = w;
  s.h = h;
  return s;
}

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Surface* screen;
SDL_Texture* map;
struct Size maxTextureSize;
int quitting = 0;
struct Coords playerPos;
struct Coords playerMove;

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
  window = SDL_CreateWindow(WINDOW_NAME,
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
  if (!window)
  {
    fprintf(stderr, "Create window failed: %s\n", SDL_GetError());
    return 0;
  }
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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
  atexit(atExitHandler);
  return 1;
}

SDL_Surface* loadImage(const char* path)
{
  SDL_Surface* loadedSurface = IMG_Load(path);
  if (!loadedSurface)
  {
    fprintf(stderr, "Unable to load image '%s': %s\n", path, IMG_GetError());
    return NULL;
  }
  SDL_Surface* optimizedSurface = SDL_ConvertSurface(loadedSurface, screen->format, 0);
  if (!optimizedSurface)
  {
    fprintf(stderr, "Unable to optimize image '%s': %s\n", path, SDL_GetError());
  }
  SDL_FreeSurface(loadedSurface);
  return optimizedSurface;
}

SDL_Texture* loadImageAsTexture(const char* path)
{
  SDL_Surface* surface = loadImage(path);
  if (!surface) return NULL;
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (!texture)
  {
    fprintf(stderr, "Unable to create texture for image '%s': %s\n", path, SDL_GetError());
  }
  SDL_FreeSurface(surface);
  return texture;
}

int loadAssets()
{
  map = loadImageAsTexture("map.png");
  if (!map) return 0;
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
    move.x -= 1;
  if (currentKeyStates[SDL_SCANCODE_Q])
    quitting = 1;
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

int updateLogic()
{
  // Apply previous move.
  playerPos.x += playerMove.x;
  playerPos.y += playerMove.y;
  // Get next move. (We need it now to interpolate.)
  playerMove = scanMoveKeys();
  // TODO: Cancel move if invalid.
}

int updateDisplay(Uint32 phase)
{
  // TODO: Interpolate object positions.
}

int draw()
{
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, map, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void pollEvents()
{
  SDL_Event e;
  if (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      quitting = 1;
    }
  }
}

int mainLoop()
{
  Uint32 startTime = SDL_GetTicks();
  Uint32 nextLogicFrameTime = 0, nextRenderFrame = 0;
  Uint32 logicFrameTimeMs = 1000 / LOGIC_FRAMES_PER_SEC;
  Uint32 renderFrameTimeMs = 1000 / RENDER_FRAMES_PER_SEC;
  while (!quitting)
  {
    // TODO: Reset frame times once per second to prevent rounding
    // error from accumulating.
    Uint32 time = SDL_GetTicks() - startTime;
    pollEvents();
    if (nextLogicFrameTime > time)
      SDL_Delay(0); // Be a little nice with the CPU when ahead of schedule.
    while (nextLogicFrameTime < time)
    {
      nextLogicFrameTime += logicFrameTimeMs;
      updateLogic();
    }
    int skipDraw = 1;
    while (nextRenderFrame < time)
    {
      nextRenderFrame += renderFrameTimeMs;
      skipDraw = 0;
    }
    if (!skipDraw)
    {
      Uint32 phase = 1000 - (nextLogicFrameTime - time) * 1000 / LOGIC_FRAMES_PER_SEC;
      updateLogic(phase);
      draw();
    }
  }
}

int main(int argc, char** argv)
{
  printf("STARTED\n");
  int success = init() && loadAssets() && mainLoop();
  printf("FINISHED\n");
  return !success;
}

