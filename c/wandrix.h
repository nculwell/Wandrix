/* vim: nu et ai ts=2 sts=2 sw=2
*/

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#define PHASE_GRAIN 4096

typedef struct Coords { int x, y; } Coords;
struct Size { int w, h; };
struct Image { 
  const char* path; SDL_Surface* sfc; SDL_Texture* tex;
};
struct CharBase {
  const char* name; struct Image img; struct Coords pos, mov; int hpCur, hpMax;
};
struct Player {
  struct CharBase c;
};
struct Npc {
  int id; // set to 0 if this slot is empty
  struct CharBase c;
};

struct TextFile {
  int nLines;
  char** lines;
};
struct IntGrid {
  Uint16 rows, cols;
  Sint16* cells;
};

enum {
  TILE_PROP_OPACITY = 0,
  TILE_PROP_OBSTACLE
};

typedef Sint8 TiledProperty;
typedef struct TiledTile {
  int id;
  int x, y;
  SDL_Texture* tex;
  TiledProperty* props;
} TiledTile;
typedef struct TiledTileset {
  Sint32 tileCount, columns, nProperties;
  struct Image image;
  char* sourceFilename;
  TiledTile* tiles;
  TiledProperty* tileProperties;
} TiledTileset;
typedef struct TiledTilesetRef {
  Sint32 firstGid;
  TiledTileset* tileset;
} TiledTilesetRef;
typedef struct TiledMap {
  Sint32 width, height, tileWidth, tileHeight, nTilesets, nLayers;
  TiledTilesetRef* tilesetRefs;
  TiledTile** layerTiles;
} TiledMap;

#define Rect_UNPACK(SDL_RECT_PTR) \
  ((SDL_RECT_PTR)->x), ((SDL_RECT_PTR)->y), ((SDL_RECT_PTR)->w), ((SDL_RECT_PTR)->h)

typedef int (*Coords_DistanceFunction)(
    struct Coords point1, struct Coords point2);

struct Coords Coords_Scale(int scalar, struct Coords s);
struct Coords Coords_Add(struct Coords a, struct Coords b);
struct Coords Coords_Sub(struct Coords a, struct Coords b);
int Coords_SimpleApproxDist(struct Coords point1, struct Coords point2);
int Coords_ApproxDist(struct Coords point1, struct Coords point2);
int Coords_ExactDist(struct Coords point1, struct Coords point2);
int Coords_FloatDist(struct Coords point1, struct Coords point2);
struct SDL_Rect Rect_Combine(struct Coords c, struct Size s);
SDL_Rect CharBase_GetRect(struct CharBase* c);
struct Size CharBase_GetSize(struct CharBase* c);
const char* Rect_ToString(SDL_Rect* r, char* buf);

void* MallocOrDie(size_t size);
int ReadBinFile(const char* filename, char** filePtr, long* fileLen);
struct TextFile* ReadTextFile(const char* filename);
void FreeTextFile(struct TextFile* lines);
struct IntGrid* ReadGridFile(const char* filename, int nRows, int nCols);
void FreeIntGrid(struct IntGrid* grid);

Sint32 ParseInt32(const char* str);
Sint16 ParseInt16(const char* str);

Sint32 SignExtend(Sint32 n);
int Abs(int n);
int SigNum(int n);

int LoadImage(struct Image* img, int createTexture);
int InitImage();

TiledMap* TiledMap_Load(const char* filename);

int InitDisplay(
    const char* windowName, int screenW, int screenH,
    int minFrameRateCap, int* frameRateCap);
int InitTileCache(TiledMap* map);
void DestroyDisplay();
void Draw(
    int phase, TiledMap* map,
    struct Player* player, struct Npc* npcs, int npcCount);

