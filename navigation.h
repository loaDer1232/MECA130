#ifndef NAVIGATION_H
#define NAVIGATION_H

typedef enum {
  TILE_NORMAL,
  TILE_FINISH,
  TILE_CHECKPOINT,
  TILE_WAYPOINT,
  TILE_HAZARD,
  TILE_START,
  TILE_UNKNOWN
} Tile;

typedef enum TileColor { BLACK, RED, GREEN, BLUE, YELLOW, WHITE } TileColor;

typedef enum Action { LEFT, FORWARD, RIGHT, IDLE } Action;
typedef enum Heading { NORTH, EAST, SOUTH, WEST } Heading;

typedef struct Cell {
  Tile type = TILE_UNKNOWN;
  bool wallNorth;
  bool wallEast;
  bool wallSouth;
  bool wallWest;
} Cell;

typedef struct Info {
  int x;
  int y;
  char heading;
  char color;
} Info;

Action decide(bool wallFront);
Tile classifyTile(TileColor color);

Info getInfo();

#endif
