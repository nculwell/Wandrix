
#include "wandrix.h"

const int MAX_INPUT_LENGTH = 260;

SDL_RWops* RWopenRead(const char* path)
{
  SDL_RWops* rw = SDL_RWFromFile(path, "rb");
  if (!rw)
    fprintf(stderr, "Unable to open file '%s': %s", path, SDL_GetError());
  return rw;
}

int RWread(struct SDL_RWops* rw, void* ptr, size_t size, size_t maxnum)
{
  size_t nRead = SDL_RWread(rw, ptr, size, maxnum);
  if (0 == nRead)
  {
    fprintf(stderr, "Error reading from file: %s", SDL_GetError());
    return 0;
  }
  return 1;
}

int ReadInts32(SDL_RWops* rw, Sint32* buf, size_t n)
{
  if (!RWread(rw, buf, sizeof(Sint32), n)) return 0;
  if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
    for (size_t i=0; i < n; ++i)
      buf[i] = SDL_SwapBE32(buf[i]);
  return 1;
}

int ReadInts16(SDL_RWops* rw, Sint16* buf, size_t n)
{
  if (!RWread(rw, buf, sizeof(Sint16), n)) return 0;
  if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
    for (size_t i=0; i < n; ++i)
      buf[i] = SDL_SwapBE16(buf[i]);
  return 1;
}

typedef struct TiledTilesetInput {
  Sint32 imageWidth, imageHeight, tileCount, columns, imageFilenameLength;
} TiledTilesetInput;

typedef struct TiledTileset {
  Sint32 tileCount, columns;
  struct Image image;
  char* sourceFilename;
} TiledTileset;

typedef struct TiledTilesetRef {
  Sint32 firstGid;
  TiledTileset* tileset;
} TiledTilesetRef;

typedef struct TiledLayer {
  Sint16 cells[0];
} TiledLayer;

typedef struct TiledMap {
  Sint32 width, height, tileWidth, tileHeight, nTilesets, nLayers;
  TiledTilesetRef* tilesetRefs;
  TiledLayer* layers;
} TiledMap;

#define MAX_LOADED_TILESETS 16
static int nLoadedTilesets = 0;
static TiledTileset loadedTilesets[MAX_LOADED_TILESETS];

static TiledTileset* LoadTileset(const char* filename)
{
  // Search for this tileset among those already loaded.
  for (int i=0; i < nLoadedTilesets; ++i)
    if (!strcmp(filename, loadedTilesets[i].sourceFilename))
      return &loadedTilesets[i];
  // Fail if the max number are already loaded.
  if (nLoadedTilesets == MAX_LOADED_TILESETS)
  {
    fprintf(stderr, "Exceeded the maximum number of loaded tilesets (%d).\n",
        MAX_LOADED_TILESETS);
    return 0;
  }
  // Load a new tileset.
  SDL_RWops* rw = RWopenRead(filename);
  if (!rw) return 0;
  TiledTileset* tileset = &loadedTilesets[nLoadedTilesets];
  ++nLoadedTilesets;
  TiledTilesetInput input;
  ReadInts32(rw, (Sint32*)&input, 5);
  char* filenameCopy = MallocOrDie(strlen(filename) + 1);
  strcpy(filenameCopy, filename);
  char imageFilename[input.imageFilenameLength + 1];
  RWread(rw, imageFilename, 1, input.imageFilenameLength);
  imageFilename[input.imageFilenameLength] = '\0';
  char* imagePath = MallocOrDie(strlen(imageFilename) + 1);
  strcpy(imagePath, imageFilename);
  tileset->image.path = imagePath;
  if (!LoadImage(&tileset->image, 1)) return 0;
  return tileset;
}

static void LoadTilesetRef(SDL_RWops* rw, TiledTilesetRef* tilesetRef)
{
  ReadInts32(rw, (Sint32*)tilesetRef, 1);
  Sint32 filenameLength;
  ReadInts32(rw, (Sint32*)&filenameLength, 1);
  assert(filenameLength < MAX_INPUT_LENGTH); // defend against rogue input
  char filename[filenameLength + 1];
  RWread(rw, filename, 1, filenameLength);
  filename[filenameLength] = '\0';
  tilesetRef->tileset = LoadTileset(filename);
}

static void LoadLayer(SDL_RWops* rw, TiledLayer* layer)
{
}

TiledMap* TiledLoadMap(const char* filename)
{
  assert(sizeof(TiledMap) == 6 * sizeof(Sint32) + 2 * sizeof(TiledLayer*));
  SDL_RWops* rw = RWopenRead(filename);
  if (!rw) return 0;
  TiledMap* map = MallocOrDie(sizeof(TiledMap));
  ReadInts32(rw, (Sint32*)map, 6);
  map->tilesetRefs = (TiledTilesetRef*)MallocOrDie(map->nTilesets * sizeof(TiledTilesetRef));
  map->layers = (TiledLayer*)MallocOrDie(map->nLayers * sizeof(TiledLayer));
  for (int i=0; i < map->nTilesets; ++i)
    LoadTilesetRef(rw, &map->tilesetRefs[i]);
  for (int i=0; i < map->nLayers; ++i)
    LoadLayer(rw, &map->layers[i]);
  return map;
}

