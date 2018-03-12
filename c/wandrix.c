/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include "wandrix.h"
#include <SDL_image.h>

const char* WINDOW_NAME = "Wandrix";
const int SCREEN_W = 800, SCREEN_H = 600;
const int FULLSCREEN = 0, VSYNC = 1;
const Uint32 LOGIC_FRAMES_PER_SEC = 16; // fixed rate
const Uint32 RENDER_FRAMES_PER_SEC = 50; // rate cap
#define NPC_COUNT 128

// Keep track of some keydown events that need to be combined with scan handling.
Uint32 keypresses;
const Uint32
  KEY_UP = 0x01,
  KEY_DOWN = 0x02,
  KEY_LEFT = 0x04,
  KEY_RIGHT = 0x08;

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Surface* screen;
struct Image map = { .path = "map.png" };
struct Coords center;
struct Size maxTextureSize;
struct Size mapImageSize;
int quitting = 0;

struct Player player = {
  { .name = "Player", .img = { .path = "y32.png" }, .pos = {0,0}, }
};

struct Npc npcs[NPC_COUNT] = {
  { .c = { .name = "Kit", .img = { .path = "ckclose32.png" }, .pos = {64,64}, } },
  { .c = { .name = "Daisy", .img = { .path = "daisy32.png" }, .pos = {96,64}, } },
  { .c = { .name = "Cindy", .img = { .path = "cstar32.png" }, .pos = {64,96}, } },
  { .c = { .name = "Desix", .img = { .path = "desix32.png" }, .pos = {96,96}, } },
};

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

int loadImage(struct Image* img, int createTexture)
{
  assert(img);
  assert(img->path);
  SDL_Surface* loadedSurface = IMG_Load(img->path);
  if (!loadedSurface)
  {
    fprintf(stderr, "Unable to load image '%s': %s\n", img->path, IMG_GetError());
    return 0;
  }
  img->sfc = SDL_ConvertSurface(loadedSurface, screen->format, 0);
  SDL_FreeSurface(loadedSurface);
  if (!img->sfc)
  {
    fprintf(stderr, "Unable to optimize image '%s': %s\n", img->path, SDL_GetError());
    return 0;
  }
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

int loadNpcs()
{
  for (int i=0; i < NPC_COUNT; ++i)
  {
    if (npcs[i].c.img.path[0])
    {
      npcs[i].id = i;
      if (!loadImage(&npcs[i].c.img, 1))
      {
        fprintf(stderr, "Unable to load NPC image.\n");
        return 0;
      }
    }
  }
  return 1;
}

int loadAssets()
{
  if (!loadImage(&map, 1)) return 0;
  mapImageSize.w = map.sfc->w;
  mapImageSize.h = map.sfc->h;
  SDL_FreeSurface(map.sfc);
  if (!loadImage(&player.c.img, 1))
  {
    fprintf(stderr, "Unable to load player image.\n");
    return 0;
  }
  if (!loadNpcs()) return 0;
  return 1;
}

struct Coords scanMoveKeys()
{
  struct Coords move = {0,0};
  const Uint8* currentKeyStates = SDL_GetKeyboardState( NULL );
  if (currentKeyStates[SDL_SCANCODE_UP] || keypresses & KEY_UP)
    move.y -= 1;
  if (currentKeyStates[SDL_SCANCODE_DOWN] || keypresses & KEY_DOWN)
    move.y += 1;
  if (currentKeyStates[SDL_SCANCODE_LEFT] || keypresses & KEY_LEFT)
    move.x -= 1;
  if (currentKeyStates[SDL_SCANCODE_RIGHT] || keypresses & KEY_RIGHT)
    move.x += 1;
  return move;
}

int detectPlayerCollision()
{
  struct Coords playerMovedPos = Coords_Add(player.c.pos, player.c.mov);
  SDL_Rect playerRect = Rect_Combine(playerMovedPos, CharBase_GetSize(&player.c));
  for (int i=0; i < NPC_COUNT; ++i)
  {
    if (npcs[i].id)
    {
      SDL_Rect npcRect = CharBase_GetRect(&npcs[i].c);
      SDL_Rect intersectRect;
      //printf("INTERSECT: PC=(%d,%d,%d,%d) NPC=(%d,%d,%d,%d)\n",
      //    playerRect.x, playerRect.y, playerRect.w, playerRect.h,
      //    npcRect.x, npcRect.y, npcRect.w, npcRect.h);
      int collide = (SDL_TRUE == SDL_IntersectRect(&playerRect, &npcRect, &intersectRect));
      if (collide) return npcs[i].id;
    }
  }
  return 0;
}

void updateLogic()
{
  struct Coords noMove = {0,0};
  // Apply previous move.
  player.c.pos = Coords_Add(player.c.pos, player.c.mov);
  // Get next move. (We need it now to interpolate.)
  player.c.mov = scanMoveKeys();
  player.c.mov = Coords_Scale(8, player.c.mov);
  // Cancel move if invalid.
  if (detectPlayerCollision())
    player.c.mov = noMove;
  // Reset keypress monitor.
  keypresses = 0;
}

void updateDisplay(Uint32 phase)
{
  // TODO: Interpolate object positions.
}

// Arguments:
// - screenRect is the position of the screen (viewport) relative to the map.
// - texture is the texture to draw.
// - textureRect is the position/size of the texture relative to the map.
// Summary:
// Finds the intersection of the viewport with the texture and draws the visible part.
int drawTexture(SDL_Rect* screenRect, SDL_Texture* texture, SDL_Rect* textureRect)
{
  SDL_Rect intersectRect;
  int intersects =
    (SDL_TRUE == SDL_IntersectRect(screenRect, textureRect, &intersectRect));
  if (intersects)
  {
    SDL_Rect sourceRect = {
      intersectRect.x - textureRect->x,
      intersectRect.y - textureRect->y,
      intersectRect.w, intersectRect.h };
    SDL_Rect screenDestRect = {
      intersectRect.x - screenRect->x,
      intersectRect.y - screenRect->y,
      intersectRect.w, intersectRect.h };
    SDL_RenderCopy(renderer, texture, &sourceRect, &screenDestRect);
  }
  return intersects;
}

void drawMap(SDL_Rect* screenRect)
{
  SDL_Rect wholeMapRect = { 0, 0, mapImageSize.w, mapImageSize.h };
  drawTexture(screenRect, map.tex, &wholeMapRect);
}

void drawChar(SDL_Rect* screenRect, struct CharBase* c)
{
  SDL_Rect charRect = {
    c->pos.x, c->pos.y,
    c->img.sfc->w, c->img.sfc->h };
  //printf("CHAR: (%d,%d)/(%d,%d) ", charRect.x, charRect.y, charRect.w, charRect.h);
  drawTexture(screenRect, c->img.tex, &charRect);
}

void drawPlayer(SDL_Rect* screenRect)
{
  drawChar(screenRect, &player.c);
}

void drawNpcs(SDL_Rect* screenRect)
{
  for (int i=0; i < NPC_COUNT; ++i)
  {
    if (npcs[i].id)
    {
      drawChar(screenRect, &npcs[i].c);
    }
  }
}

void draw()
{
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderClear(renderer);
  SDL_Rect screenRect = {
    player.c.pos.x - center.x, player.c.pos.y - center.y,
    screen->w, screen->h };
  drawMap(&screenRect);
  drawPlayer(&screenRect);
  drawNpcs(&screenRect);
  SDL_RenderPresent(renderer);
}

void handleKeypress(SDL_Event* e)
{
  switch (e->key.keysym.sym)
  {
    case SDLK_q: quitting = 1; break;
    case SDLK_UP: keypresses |= KEY_UP; break;
    case SDLK_DOWN: keypresses |= KEY_DOWN; break;
    case SDLK_LEFT: keypresses |= KEY_LEFT; break;
    case SDLK_RIGHT: keypresses |= KEY_RIGHT; break;
  }
}

void pollEvents()
{
  SDL_Event e;
  if (SDL_PollEvent(&e))
  {
    if (e.type == SDL_QUIT)
    {
      quitting = 1;
    }
    if (e.type == SDL_KEYDOWN)
    {
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
  return 1;
}

int main(int argc, char** argv)
{
  printf("STARTED\n");
  int success = init() && loadAssets() && mainLoop();
  printf("FINISHED\n");
  return !success;
}

