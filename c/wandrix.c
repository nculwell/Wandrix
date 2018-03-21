/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include "wandrix.h"

const char* WINDOW_NAME = "Wandrix";
const int SCREEN_W = 800, SCREEN_H = 600;
const Uint32 LOGIC_FRAMES_PER_SEC = 20; // fixed rate
const int MIN_FRAME_RATE_CAP = 30;
const char* MAP_MASTER_FILENAME = "map_master.txt";
#define NPC_COUNT 128

// Keep track of some keydown events that need to be combined with scan handling.
Uint32 keypresses;
const Uint32
  KEY_UP = 0x01,
  KEY_DOWN = 0x02,
  KEY_LEFT = 0x04,
  KEY_RIGHT = 0x08;

static int frameRateCap;
static int quitting = 0;

TiledMap* tiledMap = 0;

struct Player player = {
  { .name = "Player", .img = { .path = "testimg/fork32.png" },
    .pos = {1000, 600},
    //.pos = {0,0},
  }
};

struct Npc npcs[NPC_COUNT] = {
  { .c = { .name = "Kit", .img = { .path = "testimg/ckclose32.png" }, .pos = {128,128}, } },
  { .c = { .name = "Daisy", .img = { .path = "testimg/daisy32.png" }, .pos = {96,64}, } },
  { .c = { .name = "Cindy", .img = { .path = "testimg/cstar32.png" }, .pos = {64,96}, } },
  { .c = { .name = "Desix", .img = { .path = "testimg/desix32.png" }, .pos = {96,96}, } },
};

void AtExitHandler()
{
  DestroyDisplay();
  SDL_Quit();
}

int Init()
{
  if(SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
    return 0;
  }
  if (!InitImage()) return 0;
  InitDisplay(WINDOW_NAME, SCREEN_W, SCREEN_H, MIN_FRAME_RATE_CAP, &frameRateCap);
  atexit(AtExitHandler);
  tiledMap = TiledMap_Load("map.wtm");
  if (!tiledMap) return 0;
  return 1;
}

int LoadNpcs()
{
  for (int i=0; i < NPC_COUNT; ++i)
  {
    if (npcs[i].c.img.path)
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

int LoadMap()
{
  struct TextFile* master = ReadTextFile(MAP_MASTER_FILENAME);
  if (!master) return 0;
  assert(master->nLines == 5);
  //const char* tilesetFilename = master->lines[0];
  int rows = ParseInt16(master->lines[1]);
  int cols = ParseInt16(master->lines[2]);
  if (rows == SDL_MIN_SINT16 || cols == SDL_MIN_SINT16) return 0;
  struct IntGrid* tiles = ReadGridFile(master->lines[3], rows, cols);
  if (!tiles) return 0;
  struct IntGrid* cover = ReadGridFile(master->lines[4], rows, cols);
  if (!cover) return 0;
  FreeTextFile(master);
  return 1;
}

int LoadAssets()
{
  if (!LoadMap()) return 0;
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

int printLight = 1;

void HandleKeypress(SDL_Event* e)
{
  switch (e->key.keysym.sym)
  {
    case SDLK_q: quitting = 1; break;
    case SDLK_p: printf("PLAYER: (%d,%d)\n", player.c.pos.x, player.c.pos.y); break;
    case SDLK_l: printLight = 1; break;
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
         renderFrameDurationMs = frameRateCap > 0 ? 1000 / frameRateCap : 0,
         renderFramesCount = 0;
  fflush(stdout); // flush output from the init process
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
      //printf("FPS: LOGIC=%d, RENDER=%d\n", logicFramesCount, renderFramesCount);
      logicFramesCount = renderFramesCount = 0;
    }
    while (nextLogicFrameTime < time)
    {
      nextLogicFrameTime += logicFrameDurationMs;
      ++logicFramesCount;
      UpdateLogic();
    }
    int draw = 0;
    if (frameRateCap == 0)
    {
      ++renderFramesCount;
      draw = 1;
    }
    else
    {
      while (nextRenderFrame < time)
      {
        nextRenderFrame += renderFrameDurationMs;
        ++renderFramesCount;
        draw = 1;
      }
    }
    if (draw)
    {
      int phase = PHASE_GRAIN - (nextLogicFrameTime - time) * PHASE_GRAIN / logicFrameDurationMs;
      assert(phase >= 0);
      assert(phase <= PHASE_GRAIN);
      //printf("PHASE: %d\n", phase);
      Draw(phase, tiledMap, &player, npcs, NPC_COUNT);
    }
  }
  return 1;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
int WandrixMain(int argc, char** argv)
{
  printf("STARTED\n");
  int success = Init() && LoadAssets() && MainLoop();
  printf("FINISHED\n");
  return !success;
}

