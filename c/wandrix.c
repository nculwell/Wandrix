/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include "wandrix.h"
#include <SDL_image.h>
#define PHASE_GRAIN 10000

const char* WINDOW_NAME = "Wandrix";
const int SCREEN_W = 800, SCREEN_H = 600;
const int FULLSCREEN = 0, VSYNC = 1;
const Uint32 LOGIC_FRAMES_PER_SEC = 16; // fixed rate
//const Uint32 RENDER_FRAMES_PER_SEC = 60; // rate cap
const Uint32 RENDER_FRAMES_PER_SEC = 0; // rate cap
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
struct Image map = { .path = "testimg/map.png" };
struct Coords center;
struct Size maxTextureSize;
struct Size mapImageSize;
int quitting = 0;

struct Player player = {
  { .name = "Player", .img = { .path = "testimg/y32.png" }, .pos = {0,0}, }
};

struct Npc npcs[NPC_COUNT] = {
  { .c = { .name = "Kit", .img = { .path = "testimg/ckclose32.png" }, .pos = {64,64}, } },
  { .c = { .name = "Daisy", .img = { .path = "testimg/daisy32.png" }, .pos = {96,64}, } },
  { .c = { .name = "Cindy", .img = { .path = "testimg/cstar32.png" }, .pos = {64,96}, } },
  { .c = { .name = "Desix", .img = { .path = "testimg/desix32.png" }, .pos = {96,96}, } },
};

void AtExitHandler()
{
  // TODO: Destroy textures and surfaces
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

int Init()
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
  atexit(AtExitHandler);
  return 1;
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

int LoadNpcs()
{
  for (int i=0; i < NPC_COUNT; ++i)
  {
    if (npcs[i].c.img.path[0])
    {
      npcs[i].id = i;
      if (!LoadImage(&npcs[i].c.img, 1))
      {
        fprintf(stderr, "Unable to load NPC image.\n");
        return 0;
      }
    }
  }
  return 1;
}

int LoadAssets()
{
  if (!LoadImage(&map, 1)) return 0;
  mapImageSize.w = map.sfc->w;
  mapImageSize.h = map.sfc->h;
  SDL_FreeSurface(map.sfc);
  if (!LoadImage(&player.c.img, 1))
  {
    fprintf(stderr, "Unable to load player image.\n");
    return 0;
  }
  if (!LoadNpcs()) return 0;
  return 1;
}

struct Coords ScanMoveKeys()
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

int DetectPlayerCollision()
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

void UpdateLogic()
{
  struct Coords noMove = {0,0};
  // Apply previous move.
  player.c.pos = Coords_Add(player.c.pos, player.c.mov);
  // Get next move. (We need it now to interpolate.)
  player.c.mov = ScanMoveKeys();
  player.c.mov = Coords_Scale(8, player.c.mov);
  // Cancel move if invalid.
  if (DetectPlayerCollision())
    player.c.mov = noMove;
  // Reset keypress monitor.
  keypresses = 0;
}

int PRINT_IN_DRAW_TEXTURE = 0;

// Arguments:
// - screenRect is the position of the screen (viewport) relative to the map.
// - texture is the texture to draw.
// - textureRect is the position/size of the texture relative to the map.
// Summary:
// Finds the intersection of the viewport with the texture and draws the visible part.
int DrawTexture(SDL_Rect* screenRect, SDL_Texture* texture, SDL_Rect* textureRect)
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
    if (PRINT_IN_DRAW_TEXTURE) printf("SCREEN RECT: %d,%d | %d,%d\n", Rect_UNPACK(screenRect));
    if (PRINT_IN_DRAW_TEXTURE) printf("TXTURE RECT: %d,%d | %d,%d\n", Rect_UNPACK(textureRect));
    if (PRINT_IN_DRAW_TEXTURE) printf("DEST RECT:   %d,%d | %d,%d\n", Rect_UNPACK(&screenDestRect));
    SDL_RenderCopy(renderer, texture, &sourceRect, &screenDestRect);
  }
  return intersects;
}

void DrawMap(SDL_Rect* screenRect)
{
  SDL_Rect wholeMapRect = { 0, 0, mapImageSize.w, mapImageSize.h };
  DrawTexture(screenRect, map.tex, &wholeMapRect);
}

void DrawChar(SDL_Rect* screenRect, struct CharBase* c, int phase)
{
  int dx = c->mov.x * phase / PHASE_GRAIN,
      dy = c->mov.y * phase / PHASE_GRAIN;
  //if (phase==0) printf("PHASE: %4d  POS: (%d,%d)  MOV: (%d,%d)  DELTA: (%d,%d)  CHAR: %s", phase, c->pos.x, c->pos.y, c->mov.x, c->mov.y, dx, dy, c->name);
  SDL_Rect charRect = { c->pos.x + dx, c->pos.y + dy, c->img.sfc->w, c->img.sfc->h };
  //if (phase==0) printf(" RECT: (%d,%d,%d,%d)\n", charRect.x, charRect.y, charRect.w, charRect.h);
  if (phase==0) PRINT_IN_DRAW_TEXTURE=1;
  DrawTexture(screenRect, c->img.tex, &charRect);
  PRINT_IN_DRAW_TEXTURE=0;
}

void DrawPlayer(SDL_Rect* screenRect, int phase)
{
  DrawChar(screenRect, &player.c, phase);
}

void DrawNpcs(SDL_Rect* screenRect, int phase)
{
  for (int i=0; i < NPC_COUNT; ++i)
    if (npcs[i].id)
      DrawChar(screenRect, &npcs[i].c, phase);
}

void Draw(int phase)
{
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderClear(renderer);
  SDL_Rect screenRect = {
    player.c.pos.x - center.x + player.c.mov.x * phase / PHASE_GRAIN,
    player.c.pos.y - center.y + player.c.mov.y * phase / PHASE_GRAIN,
    screen->w, screen->h };
  DrawMap(&screenRect);
  DrawPlayer(&screenRect, phase);
  DrawNpcs(&screenRect, phase);
  SDL_RenderPresent(renderer);
}

void HandleKeypress(SDL_Event* e)
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

void PollEvents()
{
  SDL_Event e;
  if (SDL_PollEvent(&e))
  {
    if (e.type == SDL_QUIT)
      quitting = 1;
    if (e.type == SDL_KEYDOWN)
      HandleKeypress(&e);
  }
}

int MainLoop()
{
  Uint32 startTime = SDL_GetTicks(),
         nextLogicFrameTime = 0,
         nextRenderFrame = 0,
         nextSecond = 0,
         logicFrameDurationMs = 1000 / LOGIC_FRAMES_PER_SEC,
         logicFramesCount = 0,
         renderFramesPerSecond = RENDER_FRAMES_PER_SEC > 0 ? RENDER_FRAMES_PER_SEC : 1000,
         renderFrameDurationMs = 1000 / renderFramesPerSecond,
         renderFramesCount = 0;
  while (!quitting)
  {
    // TODO: Reset frame times once per second to prevent rounding
    // error from accumulating.
    Uint32 time = SDL_GetTicks() - startTime;
    PollEvents();
    if (nextLogicFrameTime > time)
      SDL_Delay(0); // Be a little nice with the CPU when ahead of schedule.
    while (nextSecond < time)
    {
      nextSecond += 1000;
      printf("FPS: LOGIC=%d, RENDER=%d\n", logicFramesCount, renderFramesCount);
      logicFramesCount = renderFramesCount = 0;
    }
    while (nextLogicFrameTime < time)
    {
      nextLogicFrameTime += logicFrameDurationMs;
      ++logicFramesCount;
      UpdateLogic();
    }
    int draw = 0;
    while (nextRenderFrame < time)
    {
      nextRenderFrame += renderFrameDurationMs;
      ++renderFramesCount;
      draw = 1;
    }
    if (draw)
    {
      int phase = PHASE_GRAIN - (nextLogicFrameTime - time) * PHASE_GRAIN / logicFrameDurationMs;
      assert(phase >= 0);
      assert(phase <= PHASE_GRAIN);
      printf("PHASE: %d\n", phase);
      Draw(phase);
    }
  }
  return 1;
}

int main(int argc, char** argv)
{
  printf("STARTED\n");
  int success = Init() && LoadAssets() && MainLoop();
  printf("FINISHED\n");
  return !success;
}

