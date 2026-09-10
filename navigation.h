#ifndef NAVIGATION_H
#define NAVIGATION_H

typedef enum {
  TILE_NORMAL,
  TILE_FINISH,
  TILE_CHECKPOINT,
  TILE_WAYPOINT,
  TILE_HAZARD,
  TILE_START
} Tile;

typedef enum TileColor { BLACK, RED, GREEN, BLUE, YELLOW, WHITE } TileColor;

typedef enum Action { LEFT, FORWARD, RIGHT, IDLE } Action;
typedef enum Heading { NORTH, EAST, SOUTH, WEST } Heading;

Action decide(bool wallFront);
Tile classifyTile(TileColor color);

#endif
