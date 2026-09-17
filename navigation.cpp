#include "navigation.h"
Cell maze[16][16];

int loop = 0;

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

void updateWall(Cell *cell, bool wallFront) {
  switch (heading) {
  case NORTH:
    cell->wallNorth = wallFront;
    break;
  case EAST:
    cell->wallEast = wallFront;
    break;
  case SOUTH:
    cell->wallSouth = wallFront;
    break;
  case WEST:
    cell->wallWest = wallFront;
    break;
  }
}

void updateHeading(Action direction) {
  switch (direction) {
  case LEFT:
    heading = static_cast<Heading>((heading - 1) % 4);
    break;
  case RIGHT:
    heading = static_cast<Heading>((heading + 1) % 4);
    break;
  default:
    break;
  }
}

Action decide(bool wallFront, TileColor color) {
  Cell *cell = &maze[x][y];
  // TODO implement navigation
  if (cell->type == TILE_UNKNOWN) {
    cell->type = classifyTile(color);
  }
  if (wallFront) {
    updateWall(cell, wallFront);
    updateHeading(RIGHT);
    return RIGHT;
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
    return TILE_ASSEMBLY;
  case BLUE:
    return TILE_SURVIVOR;
  case YELLOW:
    return TILE_CACHE;
  default:
    return TILE_UNKNOWN;
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
  case TILE_ASSEMBLY:
    info.color = 'g';
    break;
  case TILE_SURVIVOR:
    info.color = 'b';
    break;
  case TILE_CACHE:
    info.color = 'y';
    break;
  case TILE_UNKNOWN:
    info.color = 'c';
    break;
  default:
    info.color = 'o';
  }
  info.cell = maze[x][y];
  return info;
}
