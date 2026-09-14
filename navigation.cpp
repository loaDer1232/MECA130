#include "navigation.h"
Cell maze[16][16];

int x = 0;
int y = 0;

Heading heading = NORTH;

void updateXY() {
  switch (heading) {
  case NORTH:
    y++;
    break;
  case EAST:
    x++;
    break;
  case SOUTH:
    y--;
    break;
  case WEST:
    x--;
    break;
  }
}

void updateWall(Cell *cell) {
  switch (heading) {
  case NORTH:
    cell->wallNorth = true;
    break;
  case EAST:
    cell->wallEast = true;
    break;
  case SOUTH:
    cell->wallSouth = true;
    break;
  case WEST:
    cell->wallWest = true;
    break;
  }
}

Action decide(bool wallFront) {
  Cell *cell = &maze[x][y];
  cell->type = TILE_NORMAL;
  if (wallFront) {
    updateWall(cell);
  }
  updateXY();
  return FORWARD;
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

Info getInfo() {
  Info info;
  info.x = x;
  info.y = y;
  switch (heading) {
  case NORTH:
    info.heading = 'n';
    break;
  case EAST:
    info.heading = 'e';
    break;
  case SOUTH:
    info.heading = 's';
    break;
  case WEST:
    info.heading = 'w';
    break;
  }
  switch (maze[x][y].type) {
  case TILE_NORMAL:
    info.color = 'k';
    break;
  case TILE_HAZARD:
    info.color = 'r';
    break;
  case TILE_FINISH:
    info.color = 'g';
    break;
  case TILE_CHECKPOINT:
    info.color = 'b';
    break;
  case TILE_WAYPOINT:
    info.color = 'y';
    break;
  case TILE_UNKNOWN:
    info.color = 'c';
    break;
  default:
    info.color = 'r';
  }
  return info;
}
