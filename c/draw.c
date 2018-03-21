/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include "wandrix.h"
#include <SDL_image.h>

const double PI = 3.14159265358979323846264338327950288;

// Distance (in tiles) where light fades to zero.
// This is the limit of visibility in unobstructed terrain.
static const int VIEW_END_DISTANCE = 10;
// Distance (in tiles) where light level starts to fade out.
static const int VIEW_DROPOFF_DISTANCE = 6;
// Light levels below this threshold are rounded down to zero.
// (There are two reasons to do this. One is to optimize away drawing
// of tiles that are so dim that there's probably no point in drawing them.
// The other is to put a hard limit on visibility through semi-opaque terrain.)
static const int VIEW_LIGHT_THRESHOLD = 0x20;
// Determines whether game displays fullscreen. TODO: Make configurable.
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

int InitImage()
{
  int imgFlags = IMG_INIT_PNG;
  int imgInitResult = IMG_Init(imgFlags) & imgFlags;
  if(!imgInitResult)
  {
    fprintf(stderr, "SDL_image init failed: %s\n", IMG_GetError() );
    return 0;
  }
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
  img->sfc = loadedSurface;
  if (createTexture)
  {
    img->tex = SDL_CreateTextureFromSurface(display.renderer, img->sfc);
    if (!img->tex)
    {
      fprintf(stderr, "Unable to create texture for image '%s': %s\n", img->path, SDL_GetError());
      return 0;
    }
  }
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
  int maxVisible = nTiles - firstVisible; // number of tiles before end of map
  // might need to add one or two depending on how the intersection lines up
  while ((firstVisible + nVisible) * tileSize < intersectEdge + intersectSize)
    ++nVisible;
  // check if we went off the edge of the map
  if (nVisible > maxVisible)
    nVisible = maxVisible;
  *resultFirstVisible = firstVisible;
  *resultNVisible = nVisible;
}

extern int printLight;

void TiledMap_Draw(TiledMap* map, SDL_Rect* mapViewRect)
{
  // TODO: Draw some default tile for areas off the map edge.
  Coords mapViewCenter = {
    mapViewRect->x + display.center.x, mapViewRect->y + display.center.y };
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
  SDL_Rect tileRect = { 0, 0,  map->tileWidth, map->tileHeight };
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
      int brightness = 255;
      {
        int distance;
        {
          int dx = mapViewCenter.x - tileRect.x;
          int dy = mapViewCenter.y - tileRect.y;
          int distanceSquared = dx * dx + dy * dy;
          distance = (int)sqrt(distanceSquared);
        }
        if (distance == 0)
        {
          brightness = INT_MAX;
        }
        else
        {
          int slope = 256 / (VIEW_END_DISTANCE - VIEW_DROPOFF_DISTANCE);
          brightness = (255 + slope * VIEW_DROPOFF_DISTANCE) - (slope * distance / map->tileWidth);
          if (brightness < VIEW_LIGHT_THRESHOLD)
            brightness = 0;
        }
      }
      if (brightness == 0)
      {
        // Skip over tiles for this cell without drawing them.
        tile += map->nLayers;
      }
      else
      {
        for (int layer=0;
            layer < map->nLayers;
            ++layer, ++tile)
        {
          if (*tile)
          {
            DrawTextureWithOffset(mapViewRect,
                (*tile)->tex, &tileRect, (*tile)->x, (*tile)->y);
            TiledProperty tileOpacity = (*tile)->props[TILE_PROP_OPACITY];
            switch (tileOpacity)
            {
              case 0: break;
              case 1: brightness -= brightness >> 3; break;
              case 2: brightness -= brightness >> 2; break;
              case 3: brightness >>= 2; break;
              case 4: brightness >>= 3; break;
              case 5: brightness >>= 4; break;
              case 6: brightness >>= 5; break;
              case 7: brightness = 0; break;
              default: fprintf(stderr, "Invalid opacity level: %d\n", tileOpacity); break;
            }
          }
        }
        if (brightness < 255)
        {
          // These both return 0 on success and negative on error.
          SDL_SetRenderDrawColor(display.renderer, 0, 0, 0, 255 - brightness);
          SDL_Rect rect = {
            tileRect.x - mapViewRect->x, tileRect.y - mapViewRect->y,
            tileRect.w, tileRect.h };
          SDL_RenderFillRect(display.renderer, &rect);
        }
      }
    }
  }
  printLight = 0;
}

void DrawChar(SDL_Rect* mapViewRect, struct CharBase* c, int phase)
{
  int dx = c->mov.x * phase / PHASE_GRAIN,
      dy = c->mov.y * phase / PHASE_GRAIN;
  SDL_Rect charRect = {
    c->pos.x + dx, c->pos.y + dy,
    c->img.sfc->w, c->img.sfc->h };
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
  SDL_SetRenderDrawColor(display.renderer, 0x00, 0x00, 0x00, 0xFF);
  SDL_RenderClear(display.renderer);
  SDL_SetRenderDrawBlendMode(display.renderer, SDL_BLENDMODE_BLEND);
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

