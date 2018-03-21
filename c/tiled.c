
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

int Read4CharMarker(SDL_RWops* rw, const char* marker)
{
  assert(strlen(marker) == 4);
  char markerBuf[5] = { 0 };
  if (!RWread(rw, markerBuf, 1, 4)) return 0;
  if (0 != strcmp(marker, markerBuf))
  {
    fprintf(stderr, "Expected marker '%s' not found in file.\n", marker);
    return 0;
  }
  return 1;
}

typedef struct TiledTilesetInput {
  Sint32 imageWidth, imageHeight, tileCount, columns, nProperties, imageFilenameLength;
} TiledTilesetInput;

#define MAX_LOADED_TILESETS 16
static int nLoadedTilesets = 0;
static TiledTileset loadedTilesets[MAX_LOADED_TILESETS];

static TiledTileset* LoadTileset(const char* filename, int tileWidth, int tileHeight)
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
  //printf("Input size: %I64u, %I64u, %I64u\n", sizeof(input), sizeof(Sint32), sizeof(input) / sizeof(Sint32));
  ReadInts32(rw, (Sint32*)&input, sizeof(input) / sizeof(Sint32));
  tileset->tileCount = input.tileCount;
  tileset->columns = input.columns;
  char* filenameCopy = MallocOrDie(strlen(filename) + 1);
  strcpy(filenameCopy, filename);
  char imageFilename[input.imageFilenameLength + 1];
  RWread(rw, imageFilename, 1, input.imageFilenameLength);
  imageFilename[input.imageFilenameLength] = '\0';
  char* imagePath = MallocOrDie(strlen(imageFilename) + 1);
  strcpy(imagePath, imageFilename);
  tileset->image.path = imagePath;
  if (!LoadImage(&tileset->image, 1)) return 0;
  // Build tile objects.
  tileset->tiles = MallocOrDie(input.tileCount * sizeof(TiledTile));
  if (!Read4CharMarker(rw, "PROP")) return 0;
  tileset->nProperties = input.nProperties;
  int propertiesBufLen = input.tileCount * input.nProperties;
  tileset->tileProperties = MallocOrDie(propertiesBufLen * sizeof(*tileset->tileProperties));
  if (!RWread(rw, tileset->tileProperties, 1, propertiesBufLen)) return 0;
  int column=0, x=0, y=0, propertiesOffset=0;
  for (int t=0; t < input.tileCount; ++t, propertiesOffset += input.nProperties)
  {
    TiledTile* tile = &tileset->tiles[t];
    tile->x = x;
    tile->y = y;
    tile->tex = tileset->image.tex;
    tile->props = &tileset->tileProperties[propertiesOffset];
    ++column;
    if (column == tileset->columns)
    {
      column = 0;
      x = 0;
      y += tileHeight;
    }
    else
    {
      x += tileWidth;
    }
  }
  // TODO: Check that we're at the end of the file.
  return tileset;
}

static int LoadTilesetRef(SDL_RWops* rw, TiledTilesetRef* tilesetRef,
    Sint32 tileWidth, Sint32 tileHeight)
{
  ReadInts32(rw, (Sint32*)tilesetRef, 1);
  Sint32 filenameLength;
  ReadInts32(rw, (Sint32*)&filenameLength, 1);
  assert(filenameLength > 0); // defend against rogue input
  assert(filenameLength < MAX_INPUT_LENGTH); // defend against rogue input
  char filename[filenameLength + 1];
  RWread(rw, filename, 1, filenameLength);
  filename[filenameLength] = '\0';
  printf("TS filename length: %I64d/%d\n", strlen(filename), filenameLength);
  tilesetRef->tileset = LoadTileset(filename, tileWidth, tileHeight);
  if (!tilesetRef->tileset) return 0;
  return 1;
}

static TiledTile* TiledMap_FindTile(TiledMap* map, Sint16 gid)
{
  if (gid == 0)
    return 0; // space has no tile in this layer
  for (int ts = map->nTilesets - 1; ts >= 0; --ts)
  {
    TiledTilesetRef* ref = &map->tilesetRefs[ts];
    TiledTileset* tileset = ref->tileset;
    int tileIndex = gid - ref->firstGid;
    printf("%p\n", tileset); fflush(stdout);
    if (tileIndex < tileset->tileCount)
    {
      TiledTile* tile = &tileset->tiles[tileIndex];
      //printf("Found tile: %d (%d,%d)\n", tileIndex, tile->x, tile->y);
      return tile;
    }
  }
  fprintf(stderr, "Tile not found for GID %d.\n", gid);
  return 0;
}

TiledMap* TiledMap_Load(const char* filename)
{
  SDL_RWops* rw = RWopenRead(filename);
  if (!rw) return 0;
  TiledMap* map = MallocOrDie(sizeof(TiledMap));
  ReadInts32(rw, (Sint32*)map, 6);
  printf("Read values: w=%d, h=%d,"
      " tw=%d, th=%d, nt=%d, nl=%d\n",
      map->width, map->height,
      map->tileWidth, map->tileHeight, map->nTilesets, map->nLayers);
  map->tilesetRefs = MallocOrDie(map->nTilesets * sizeof(TiledTilesetRef));
  for (int i=0; i < map->nTilesets; ++i)
    if (!LoadTilesetRef(rw, &map->tilesetRefs[i], map->tileWidth, map->tileHeight))
      return 0;
  size_t singleLayerCellCount = map->width * map->height;
  size_t totalCellCount = map->nLayers * singleLayerCellCount;
  Sint16* tileGids = MallocOrDie(totalCellCount * sizeof(Sint16));
  if (!ReadInts16(rw, tileGids, totalCellCount)) return 0;
  TiledTile** tiles = MallocOrDie(totalCellCount * sizeof(TiledTile**));
  for (size_t t=0; t < totalCellCount; ++t)
  {
    int gid = tileGids[t];
    tiles[t] = TiledMap_FindTile(map, gid);
  }
  free(tileGids);
  map->layerTiles = tiles;
  return map;
}

