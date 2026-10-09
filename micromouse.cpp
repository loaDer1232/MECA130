#ifdef Micromouse
#include "./API/API.h"
#include "GlobalDefs.h"
#include "navigation.h"

#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>

TileColor mazeTile[16][16] = {BLACK};

static void placeTiles(int count, TileColor tile, char apiColor, int width,
                       int height) {
  for (int i = 0; i < count; i++) {
    int x, y;
    do {
      x = rand() % width;
      y = rand() % height;
    } while ((x == 0 && y == 0) || mazeTile[x][y] != BLACK);
    mazeTile[x][y] = tile;
    API_setColor(x, y, apiColor);
  }
}

void mazeInit(int width, int height) {
  mazeTile[0][0] = WHITE;
  API_setColor(0, 0, 'w');
  placeTiles(NUM_RED_TILES, RED, 'r', width, height);
  placeTiles(NUM_GRN_TILES, GREEN, 'g', width, height);
  placeTiles(NUM_BLU_TILES, BLUE, 'b', width, height);
  placeTiles(NUM_YEL_TILES, YELLOW, 'y', width, height);
}

void plotWalls(int x, int y, Cell cell) {
  if (cell.wallNorth)
    API_setWall(x, y, 'n');
  if (cell.wallEast)
    API_setWall(x, y, 'e');
  if (cell.wallSouth)
    API_setWall(x, y, 's');
  if (cell.wallWest)
    API_setWall(x, y, 'w');
}

char *makeDebugStr(int x, int y, int targetX, int targetY, char color,
                   char heading, int surviors, int caches, int hazards) {
  char *str = (char *)malloc(256 * sizeof(char));
  if (str != NULL) {
    sprintf(str,
            "Heading: %c, x:%i, y:%i, Target: x:%i, y:%i color:%c\n surviors "
            "found: %i/%i, caches "
            "found: %i/%i Hazards found: %i/%i",
            heading, x, y, targetX, targetY, color, surviors, NUM_SURVIVORS,
            caches, NUM_CACHE, hazards, NUM_HAZARD);
  }
  return str;
}

int main() {
  mazeInit(API_mazeWidth(), API_mazeHeight());
  bool done = false;
  while (!done) {
    Info info = getInfo();
    Action decsion =
        decide(API_wallFront(), mazeTile[info.cell.x][info.cell.y]);
    plotWalls(info.cell.x, info.cell.y, info.cell);
    switch (decsion) {
    case FORWARD:
      API_setColor(info.cell.x, info.cell.y, info.color);
      API_moveForward();
      debug_log(const_cast<char *>("Moving forward"));
      break;
    case LEFT:
      API_turnLeft();
      debug_log(const_cast<char *>("Turning left"));
      break;
    case RIGHT:
      API_turnRight();
      debug_log(const_cast<char *>("Turing right"));
      break;
    case REVERSE:
      API_turnRight();
      API_turnRight();
      API_moveForward();
      API_turnRight();
      API_turnRight();
      break;
    case IDLE:
      debug_log(const_cast<char *>("Exploration finished"));
      done = true;
      break;
    default:
      debug_log(const_cast<char *>("undefined state"));
      break;
    }
    char *str = makeDebugStr(
        info.cell.x, info.cell.y, info.target.x, info.target.y, info.color,
        info.heading, info.survivorsFound, info.cachesFound, info.hazardsFound);
    debug_log(str);
    free(str);
  }
  return 0;
}
#endif
