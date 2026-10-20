#ifndef NAVIGATION_H
#define NAVIGATION_H
#include "GlobalDefs.h"

typedef enum Tile {
  TILE_NORMAL,
  TILE_ASSEMBLY,
  TILE_SURVIVOR,
  TILE_CACHE,
  TILE_HAZARD,
  TILE_START,
  TILE_UNKNOWN
} Tile;

typedef enum Action { LEFT, FORWARD, RIGHT, IDLE, REVERSE, FLIP } Action;
typedef enum Heading { NORTH, EAST, SOUTH, WEST } Heading;
typedef enum Wall { FALSE, TRUE, UNKNOWN } Wall;

typedef struct Cell {
  Tile type = TILE_UNKNOWN;
  Wall wallNorth = UNKNOWN;
  Wall wallEast = UNKNOWN;
  Wall wallSouth = UNKNOWN;
  Wall wallWest = UNKNOWN;
  int x;
  int y;
} Cell;

typedef struct Info {
  int survivorsFound;
  int cachesFound;
  int hazardsFound;
  char heading;
  char color;
  Cell cell;
  Cell target;
} Info;

struct AssmeblyTile {
  bool found = false;
  Cell *cell;
};

Action decide(bool wallFront, TileColor color);
Tile classifyTile(TileColor color);

Info getInfo();
Heading getHeading();

#endif
