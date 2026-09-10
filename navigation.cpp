#include "navigation.h"

Action decide(bool wallFront) {
  if (wallFront) {
    return LEFT;
  } else {
    return FORWARD;
  }
}

Tile classifyTile(TileColor color) {
  switch (color) {
  case BLACK:
    return TILE_NORMAL;
  case RED:
    return TILE_HAZARD;
  case GREEN:
    return TILE_FINISH;
  case BLUE:
    return TILE_CHECKPOINT;
  case YELLOW:
    return TILE_WAYPOINT;
  default:
    return TILE_NORMAL;
  }
}
