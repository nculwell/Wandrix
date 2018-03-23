/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include "wandrix.h"
#include <SDL_image.h>

SDL_Surface* CreateQuarterCircle(int radius, Uint32 colorRGBA);

const double PI = 3.14159265358979323846264338327950288;

static const int BORDER_THICKNESS = 32;

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
// Distance across the entire view, in tiles. This is used to determine
// the necessary size of the view on the screen.
static int VIEW_DIAMETER;
static int VIEW_CENTER;
static const int MAX_LIGHT = 0xFF;

// Determines whether game displays fullscreen. TODO: Make configurable.
static const int FULLSCREEN = 0;

static struct Display {
  SDL_Window* window;
  SDL_Surface* screen;
  SDL_Renderer* renderer;
  TiledTile** tileCache;
  int* tileLighting;
  Coords tileCacheMapPos;
} display;

static struct Layout {
  int isHorizontal;
  SDL_Rect mapDisplayRect, uiDisplayRect;
  SDL_Rect divider;
  SDL_Rect borderTop, borderBottom, borderLeft, borderRight;
  SDL_Rect cornerTopLeft, cornerTopRight, cornerBottomLeft, cornerBottomRight;
} layout;

static SDL_Texture* quarterCircle;

#define RGB_TO_RGBA(COLOR) (((COLOR) << 8) | 0xFF)

const Uint32 COLOR_FRAME = 0x5455FF;
const Uint32 COLOR_UI_PANE = 0x202040;
const Uint32 COLOR_BLACK = 0;

// PROTOTYPES
void CalculateLight(int tileX, int tileY, Coords tileShift, TiledMap* map);

void SetColor(Uint32 color)
{
  Uint32 red   = (color >> 16) & 0xFF;
  Uint32 green = (color >>  8) & 0xFF;
  Uint32 blue  =  color        & 0xFF;
  SDL_SetRenderDrawColor(display.renderer, red, green, blue, 0xFF);
}

void DestroyDisplay()
{
  // TODO: Destroy textures and surfaces
  SDL_DestroyRenderer(display.renderer);
  SDL_DestroyWindow(display.window);
}

SDL_Texture* SurfaceToTexture(SDL_Surface* surface, int freeSurfaceWhenDone)
{
  SDL_Texture* texture = SDL_CreateTextureFromSurface(display.renderer, surface);
  if (!texture)
    fprintf(stderr, "Unable to create texture from surface. %s\n", SDL_GetError());
  if (freeSurfaceWhenDone)
    SDL_FreeSurface(surface);
  return texture;
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
  display.screen = SDL_GetWindowSurface(display.window);
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
  SDL_Surface* quarterCircleSurface = CreateQuarterCircle(BORDER_THICKNESS, RGB_TO_RGBA(COLOR_FRAME));
  quarterCircle = SurfaceToTexture(quarterCircleSurface, 1);
  if (!quarterCircle)
  {
    fprintf(stderr, "Failed to convert circle surface to texture.\n");
    return 0;
  }
  VIEW_DIAMETER = 2 * VIEW_END_DISTANCE + 1;
  VIEW_CENTER = VIEW_DIAMETER / 2;
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

int InitTileCache(TiledMap* map)
{
  assert(map);
  int cacheSize = 2 * VIEW_END_DISTANCE + 1;
  display.tileCache = MallocOrDie(cacheSize * map->nLayers * sizeof(TiledTile**));
  display.tileLighting = MallocOrDie(cacheSize * sizeof(int));
  return 1;
}

int LoadImage(struct Image* img, int createTexture)
{
  assert(img);
  assert(img->path);
  SDL_Surface* loadedSurface = IMG_Load(img->path);
  if (!loadedSurface)
  {
    fprintf(stderr, "Failed to load image '%s'. %s\n", img->path, IMG_GetError());
    return 0;
  }
  img->sfc = loadedSurface;
  if (createTexture)
  {
    img->tex = SurfaceToTexture(img->sfc, 0);
    if (!img->tex)
    {
      fprintf(stderr, "Failed to create texture for image '%s'.\n", img->path);
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

int ReduceBrightness(int brightness, int tileOpacity)
{
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
  return brightness;
}

void BuildTileCache(TiledMap* map, SDL_Rect* mapViewRect)
{
  Coords mapViewCenter = {
    mapViewRect->x + mapViewRect->w / 2, mapViewRect->y + mapViewRect->h / 2 };
  Coords centerTile = { mapViewCenter.x / map->tileWidth, mapViewCenter.y / map->tileHeight };
  Coords cacheCenterPoint = {
    centerTile.x * map->tileWidth + map->tileWidth / 2,
    centerTile.y * map->tileHeight + map->tileHeight / 2 };
  Coords tileShift = Coords_Sub(mapViewCenter, cacheCenterPoint);
  int firstVisibleRow = centerTile.y - VIEW_END_DISTANCE;
  int firstVisibleCol = centerTile.x - VIEW_END_DISTANCE;
  display.tileCacheMapPos.x = firstVisibleCol * map->tileWidth;
  display.tileCacheMapPos.y = firstVisibleRow * map->tileHeight;
  int nVisibleRows = 2 * VIEW_END_DISTANCE + 1;
  int nVisibleCols = nVisibleRows;
  int nTilesInRow = map->nLayers * map->width;
  int firstTileOffset = nTilesInRow * firstVisibleRow + map->nLayers * firstVisibleCol;
  TiledTile** nextRowFirstTile = map->layerTiles + firstTileOffset;
  TiledTile** tileCachePtr = display.tileCache;
  for (int r=0;
      r < 2 * VIEW_END_DISTANCE + 1;
      ++r)
  {
    TiledTile** tile = nextRowFirstTile;
    nextRowFirstTile += nTilesInRow;
    int mapRow = firstVisibleRow + r;
    for (int c=0;
        c < 2 * VIEW_END_DISTANCE + 1;
        ++c)
    {
      int mapCol = firstVisibleCol + nVisibleCols;
      for (int layer=0;
          layer < map->nLayers;
          ++layer, ++tile, ++tileCachePtr)
      {
        if (mapRow < 0 || mapRow >= map->height
            || mapCol < 0 || mapCol >= map->width)
        {
          *tileCachePtr = 0;
        }
        else
        {
          *tileCachePtr = *tile;
        }
      }
    }
  }
  // The tile that the player is standing on is always fully lit.
  display.tileLighting[VIEW_DIAMETER * VIEW_CENTER + VIEW_CENTER] = MAX_LIGHT;
  for (int radius=1; radius <= VIEW_END_DISTANCE; ++radius)
  {
    for (int t=0; t <= radius; ++t)
    {
      CalculateLight(centerTile.x + t, centerTile.y + radius, tileShift, map);
      CalculateLight(centerTile.x + radius, centerTile.y + t, tileShift, map);
      CalculateLight(centerTile.x - t, centerTile.y - radius, tileShift, map);
      CalculateLight(centerTile.x - radius, centerTile.y - t, tileShift, map);
    }
  }
}

TiledTile** GetTile(TiledMap* map, int x, int y)
{
  int tileOffset = (y * map->width + x) * map->nLayers;
  TiledTile** tile = map->layerTiles + tileOffset;
  return tile;
}

void CalculateLight(int tileX, int tileY, Coords tileShift, TiledMap* map)
{
  int dx = (VIEW_CENTER - tileX) * map->tileWidth + tileShift.x;
  int dy = (VIEW_CENTER - tileY) * map->tileHeight + tileShift.y;
  int dxSquare = dx * dx;
  int dySquare = dy * dy;
  int distance = (int)sqrt(dxSquare + dySquare);
  int nextTileX, nextTileY;
  if (dxSquare > dySquare)
  {
    nextTileX = tileX - SigNum(dx);
    nextTileY = tileY;
  }
  else
  {
    nextTileX = tileX;
    nextTileY = tileY - SigNum(dy);
  }
  int brightness = MAX_LIGHT;
  TiledTile** tile = GetTile(map, nextTileX, nextTileY);
  for (int layer=0; layer < map->nLayers; ++layer, ++tile)
  {
    if (*tile)
    {
      TiledProperty tileOpacity = (*tile)->props[TILE_PROP_OPACITY];
      brightness = ReduceBrightness(brightness, tileOpacity);
    }
  }
  int slope = 256 / (VIEW_END_DISTANCE - VIEW_DROPOFF_DISTANCE);
  if (distance > VIEW_DROPOFF_DISTANCE)
  {
    brightness = (brightness + slope * VIEW_DROPOFF_DISTANCE)
      - (slope * distance / map->tileWidth);
  }
  if (brightness < VIEW_LIGHT_THRESHOLD)
    brightness = 0;
  display.tileLighting[VIEW_DIAMETER * tileX + tileY] = brightness;
}

void TiledMap_Draw(TiledMap* map, SDL_Rect* mapViewRect)
{
  // TODO: Rebuild tile cache only when necessary.
  BuildTileCache(map, mapViewRect);
  // TODO: Draw some default tile for areas off the map edge.
  TiledTile** tile = display.tileCache;
  SDL_Rect tileRectStart = {
    display.tileCacheMapPos.x, display.tileCacheMapPos.y,
    map->tileWidth, map->tileHeight
  };
  SDL_Rect tileRect = tileRectStart;
  int* tileBrightness = display.tileLighting;
  for (int r=0; r < VIEW_DIAMETER; ++r, tileRect.y += map->tileHeight)
  {
    tileRect.x = tileRectStart.x;
    for (int c=0; c < VIEW_DIAMETER; ++c, tileRect.x += map->tileWidth, ++tileBrightness)
    {
      int brightness = *tileBrightness;
      if (brightness == 0)
      {
        tile += 2;
        continue;
      }
      for (int layer=0; layer < map->nLayers; ++layer, ++tile)
      {
        if (*tile && brightness > 0)
        {
          DrawTextureWithOffset(mapViewRect,
              (*tile)->tex, &tileRect, (*tile)->x, (*tile)->y);
        }
      }
      if (brightness < 255)
      {
        // These both return 0 on success and negative on error.
        SDL_SetRenderDrawColor(display.renderer, 0, 0, 0, 255 - brightness);
        SDL_Rect shadeRect = {
          tileRect.x - mapViewRect->x, tileRect.y - mapViewRect->y,
          tileRect.w, tileRect.h };
        SDL_Rect clipRect;
        if (SDL_TRUE == SDL_IntersectRect(&layout.mapDisplayRect, &shadeRect, &clipRect))
          SDL_RenderFillRect(display.renderer, &clipRect);
      }
    }
  }
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

void DrawUi()
{
  SetColor(COLOR_UI_PANE);
  SDL_RenderFillRect(display.renderer, &layout.uiDisplayRect);
}

void ComputeLayout()
{
  // TODO: Just call this once after resize.
  int border = BORDER_THICKNESS;
  int halfBorder = BORDER_THICKNESS / 2;
  int doubleBorder = 2 * BORDER_THICKNESS;
  int screenW = display.screen->w;
  int screenH = display.screen->h;
  layout.isHorizontal = (screenW >= screenH);
  int mapViewSize = (layout.isHorizontal ? screenH : screenW) - doubleBorder;
  layout.mapDisplayRect.x = border;
  layout.mapDisplayRect.y = border;
  layout.mapDisplayRect.w = mapViewSize;
  layout.mapDisplayRect.h = mapViewSize;
  int uiDisplayShift = mapViewSize + doubleBorder;
  if (layout.isHorizontal)
  {
    layout.uiDisplayRect.x = uiDisplayShift;
    layout.uiDisplayRect.y = border;
    layout.uiDisplayRect.w = screenW - uiDisplayShift - border;
    layout.uiDisplayRect.h = screenH - doubleBorder;
    layout.divider.x = border + mapViewSize;
    layout.divider.y = border;
    layout.divider.w = border;
    layout.divider.h = screenH - doubleBorder;
  }
  else
  {
    layout.uiDisplayRect.x = border;
    layout.uiDisplayRect.y = uiDisplayShift;
    layout.uiDisplayRect.w = screenW - doubleBorder;
    layout.uiDisplayRect.h = screenH - uiDisplayShift - border;
    layout.divider.x = border;
    layout.divider.y = border + mapViewSize;
    layout.divider.w = screenW - doubleBorder;
    layout.divider.h = border;
  };
  SDL_Rect borderTop = { halfBorder, 0, screenW - border, border };
  SDL_Rect borderBottom = { halfBorder, screenH - border, screenW - border, border };
  SDL_Rect borderLeft = { 0, halfBorder, border, screenH - border };
  SDL_Rect borderRight = { screenW - border, halfBorder, border, screenH - border };
  SDL_Rect cornerTopLeft = { 0, 0, halfBorder, halfBorder };
  SDL_Rect cornerTopRight = { screenW - halfBorder, 0, halfBorder, halfBorder };
  SDL_Rect cornerBottomLeft = { 0, screenH - halfBorder, halfBorder, halfBorder };
  SDL_Rect cornerBottomRight = { screenW - halfBorder, screenH - halfBorder, halfBorder, halfBorder };
  layout.borderTop = borderTop;
  layout.borderBottom = borderBottom;
  layout.borderLeft = borderLeft;
  layout.borderRight = borderRight;
  layout.cornerTopLeft = cornerTopLeft;
  layout.cornerTopRight = cornerTopRight;
  layout.cornerBottomLeft = cornerBottomLeft;
  layout.cornerBottomRight = cornerBottomRight;
}

void DrawLayout()
{
  // Draw frame.
  SetColor(COLOR_FRAME);
  SDL_RenderFillRect(display.renderer, &layout.borderTop);
  SDL_RenderFillRect(display.renderer, &layout.borderBottom);
  SDL_RenderFillRect(display.renderer, &layout.borderLeft);
  SDL_RenderFillRect(display.renderer, &layout.borderRight);
  SDL_RenderFillRect(display.renderer, &layout.divider);
  SDL_RenderCopyEx(display.renderer, quarterCircle, 0, &layout.cornerTopLeft,
      0, 0, SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
  SDL_RenderCopyEx(display.renderer, quarterCircle, 0, &layout.cornerTopRight,
      0, 0, SDL_FLIP_VERTICAL);
  SDL_RenderCopyEx(display.renderer, quarterCircle, 0, &layout.cornerBottomLeft,
      0, 0, SDL_FLIP_HORIZONTAL);
  SDL_RenderCopyEx(display.renderer, quarterCircle, 0, &layout.cornerBottomRight,
      0, 0, SDL_FLIP_NONE);
}

void Draw(int phase, TiledMap* map, struct Player* player, struct Npc* npcs, int npcCount)
{
  SDL_SetRenderDrawColor(display.renderer, 0x00, 0x00, 0x00, 0xFF);
  SDL_RenderClear(display.renderer);
  SDL_SetRenderDrawBlendMode(display.renderer, SDL_BLENDMODE_BLEND);
  //SDL_SetRenderDrawColor(display.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  ComputeLayout();
  DrawLayout();
  SDL_Rect mapViewRect;
  mapViewRect.w = map->tileWidth * VIEW_DIAMETER;
  mapViewRect.h = map->tileHeight * VIEW_DIAMETER;
  mapViewRect.x = player->c.pos.x - mapViewRect.w / 2
    + player->c.mov.x * phase / PHASE_GRAIN;
  mapViewRect.y = player->c.pos.y - mapViewRect.h / 2
    + player->c.mov.y * phase / PHASE_GRAIN;
  TiledMap_Draw(map, &mapViewRect);
  DrawPlayer(&mapViewRect, phase, player);
  DrawNpcs(&mapViewRect, phase, npcs, npcCount);
  DrawUi();
  SDL_RenderPresent(display.renderer);
}

