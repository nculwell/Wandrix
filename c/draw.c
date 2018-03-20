/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include "wandrix.h"

static const int FULLSCREEN = 1;

struct Display {
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Surface* screen;
  struct Coords center;
  struct Size maxTextureSize;
};

static struct Display display;

void DestroyDisplay()
{
  // TODO: Destroy textures and surfaces
  SDL_DestroyRenderer(display.renderer);
  SDL_DestroyWindow(display.window);
}

int InitDisplay(
    const char* windowName, int screenW, int screenH,
    int minFrameRateCap, int* frameRateCap)
{
  Uint32 windowFlags = (FULLSCREEN * SDL_WINDOW_FULLSCREEN_DESKTOP);
  int wPos = SDL_WINDOWPOS_CENTERED;
  display.window = SDL_CreateWindow(windowName, wPos, wPos, screenW, screenH, windowFlags);
  if (!display.window)
  {
    fprintf(stderr, "Create window failed: %s\n", SDL_GetError());
    return 0;
  }
  SDL_DisplayMode displayMode;
  if (0 != SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(display.window), &displayMode))
  {
    fprintf(stderr, "GetCurrentDisplayMode failed: %s\n", SDL_GetError());
    return 0;
  }
  int vsync;
  if (displayMode.refresh_rate >= minFrameRateCap)
  {
    *frameRateCap = 0;
    vsync = 1;
  }
  else
  {
    *frameRateCap = displayMode.refresh_rate;
    while (*frameRateCap < minFrameRateCap)
      *frameRateCap *= 2;
    vsync = 0;
  }
  display.renderer = SDL_CreateRenderer(display.window, -1,
      SDL_RENDERER_ACCELERATED | (vsync * SDL_RENDERER_PRESENTVSYNC));
  if (!display.renderer)
  {
    fprintf(stderr, "Create renderer failed: %s\n", SDL_GetError());
    return 0;
  }
  SDL_RendererInfo rendererInfo;
  if (0 > SDL_GetRendererInfo(display.renderer, &rendererInfo))
  {
    fprintf(stderr, "Get renderer info failed: %s\n", SDL_GetError());
    return 0;
  }
  display.maxTextureSize.w = rendererInfo.max_texture_width;
  display.maxTextureSize.h = rendererInfo.max_texture_height;
  printf("Max texture size: %dx%d\n", display.maxTextureSize.w, display.maxTextureSize.h);
  display.screen = SDL_GetWindowSurface(display.window);
  display.center.x = display.screen->w / 2;
  display.center.y = display.screen->h / 2;
  return 1;
}

int DrawTextureWithOffset(SDL_Rect* mapViewRect, SDL_Texture* texture,
    SDL_Rect* textureRect, int textureOffsetX, int textureOffsetY)
{
  SDL_Rect intersectRect;
  int intersects =
    (SDL_TRUE == SDL_IntersectRect(mapViewRect, textureRect, &intersectRect));
  if (intersects)
  {
    SDL_Rect sourceRect = {
      intersectRect.x - textureRect->x + textureOffsetX,
      intersectRect.y - textureRect->y + textureOffsetY,
      intersectRect.w, intersectRect.h };
    SDL_Rect screenDestRect = {
      intersectRect.x - mapViewRect->x,
      intersectRect.y - mapViewRect->y,
      intersectRect.w, intersectRect.h };
    SDL_RenderCopy(display.renderer, texture, &sourceRect, &screenDestRect);
  }
  return intersects;
}

// Arguments:
// - mapViewRect is the position of the screen (viewport) relative to the map.
// - texture is the texture to draw.
// - textureRect is the position/size of the texture relative to the map.
// Summary:
// Finds the intersection of the viewport with the texture and draws the visible part.
int DrawTexture(SDL_Rect* mapViewRect, SDL_Texture* texture, SDL_Rect* textureRect)
{
  return DrawTextureWithOffset(mapViewRect, texture, textureRect, 0, 0);
}

static void ComputeVisibleCoords(int* resultFirstVisible, int* resultNVisible,
    int nTiles, int tileSize, int intersectEdge, int intersectSize)
{
  int firstVisible = intersectEdge / tileSize;
  if (firstVisible * tileSize > intersectEdge)
    --firstVisible;
  int nVisible = intersectSize / tileSize;
  while ((firstVisible + nVisible) * tileSize < intersectEdge + intersectSize)
    ++nVisible;
  *resultFirstVisible = firstVisible;
  *resultNVisible = nVisible;
}

void TiledMap_Draw(TiledMap* map, SDL_Rect* mapViewRect)
{
  int mapWidthPixels = map->width * map->tileWidth;
  int mapHeightPixels = map->height * map->tileHeight;
  SDL_Rect wholeMapRect = { 0, 0, mapWidthPixels, mapHeightPixels };
  SDL_Rect intersectRect;
  if (SDL_TRUE != SDL_IntersectRect(mapViewRect, &wholeMapRect, &intersectRect))
    return;
  int firstVisibleRow, nVisibleRows, firstVisibleCol, nVisibleCols;
  ComputeVisibleCoords(&firstVisibleRow, &nVisibleRows,
      map->height, map->tileHeight, intersectRect.y, intersectRect.h);
  ComputeVisibleCoords(&firstVisibleCol, &nVisibleCols,
      map->width, map->tileWidth, intersectRect.x, intersectRect.w);
  // TODO: Only iterate over visible tiles.
  SDL_Rect tileRect = { 0, 0,  map->tileWidth, map->tileHeight };
  //TiledTile* tile = &map->layers->tiles[0];
  int nTilesInRow = map->nLayers * map->width;
  int firstTileOffset = nTilesInRow * firstVisibleRow + map->nLayers * firstVisibleCol;
  //printf("nTilesInRow=%d, firstTileOffset=%d\n", nTilesInRow, firstTileOffset);
  TiledTile** nextRowFirstTile = map->layerTiles + firstTileOffset;
  int firstTileX = firstVisibleCol * map->tileWidth;
  tileRect.y = firstVisibleRow * map->tileHeight;
  for (int r = firstVisibleRow;
      r < firstVisibleRow + nVisibleRows;
      ++r, tileRect.y += map->tileHeight)
  {
    TiledTile** tile = nextRowFirstTile;
    nextRowFirstTile += nTilesInRow;
    tileRect.x = firstTileX;
    for (int c = firstVisibleCol;
        c < firstVisibleCol + nVisibleCols;
        ++c, tileRect.x += map->tileWidth)
    {
      for (int layer=0; layer < map->nLayers; ++layer)
      {
        if (*tile)
        {
          DrawTextureWithOffset(mapViewRect,
              (*tile)->tex, &tileRect, (*tile)->x, (*tile)->y);
        }
        ++tile;
      }
    }
  }
}

void DrawChar(SDL_Rect* mapViewRect, struct CharBase* c, int phase)
{
  int dx = c->mov.x * phase / PHASE_GRAIN,
      dy = c->mov.y * phase / PHASE_GRAIN;
  SDL_Rect charRect = { c->pos.x + dx, c->pos.y + dy, c->img.sfc->w, c->img.sfc->h };
  DrawTexture(mapViewRect, c->img.tex, &charRect);
}

void DrawPlayer(SDL_Rect* mapViewRect, int phase, struct Player* player)
{
  DrawChar(mapViewRect, &player->c, phase);
}

void DrawNpcs(SDL_Rect* mapViewRect, int phase, struct Npc* npcs, int npcCount)
{
  for (int i=0; i < npcCount; ++i)
    if (npcs[i].id)
      DrawChar(mapViewRect, &npcs[i].c, phase);
}

void Draw(int phase, TiledMap* map, struct Player* player, struct Npc* npcs, int npcCount)
{
  SDL_SetRenderDrawColor(display.renderer, 0x40, 0x40, 0x40, 0xFF);
  SDL_RenderClear(display.renderer);
  SDL_SetRenderDrawColor(display.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_Rect mapViewRect = {
    player->c.pos.x - display.center.x + player->c.mov.x * phase / PHASE_GRAIN,
    player->c.pos.y - display.center.y + player->c.mov.y * phase / PHASE_GRAIN,
    display.screen->w, display.screen->h };
  TiledMap_Draw(map, &mapViewRect);
  DrawPlayer(&mapViewRect, phase, player);
  DrawNpcs(&mapViewRect, phase, npcs, npcCount);
  SDL_RenderPresent(display.renderer);
}

