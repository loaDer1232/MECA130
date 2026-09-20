#ifndef NAVIGATION_H
#define NAVIGATION_H

typedef enum {
  TILE_NORMAL,
  TILE_ASSEMBLY,
  TILE_SURVIVOR,
  TILE_CACHE,
  TILE_HAZARD,
  TILE_START,
  TILE_UNKNOWN
} Tile;

typedef enum TileColor { BLACK, RED, GREEN, BLUE, YELLOW, WHITE } TileColor;

typedef enum Action { LEFT, FORWARD, RIGHT, IDLE, REVERSE } Action;
typedef enum Heading { NORTH, EAST, SOUTH, WEST } Heading;

typedef struct Cell {
  Tile type = TILE_UNKNOWN;
  bool wallNorth;
  bool wallEast;
  bool wallSouth; 
  bool wallWest;
  int x;
  int y;
} Cell;

typedef struct Info {
  int x;
  int y;
  int survivorsFound;
  int cachesFound;
  int hazardsFound;
  char heading;
  char color;
  Cell cell;
} Info;

Action decide(bool wallFront, TileColor color);
Tile classifyTile(TileColor color);

Info getInfo();

#endif
