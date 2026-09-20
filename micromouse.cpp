#include "API/API.h"
#include "navigation.h"
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>

#define NUM_RED_TILES 3
#define NUM_YEL_TILES 4
#define NUM_BLU_TILES 3
#define NUM_GRN_TILES 1

TileColor mazeTile[16][16] = {BLACK};

void mazeInit(int width, int height) {
  mazeTile[0][0] = WHITE;
  API_setColor(0, 0, 'w');
  for (size_t i = 0; i < NUM_RED_TILES; i++) {
    int x = (rand() % width) + 1;
    int y = (rand() % height) + 1;
    mazeTile[x][y] = RED;
    API_setColor(x, y, 'r');
  }
  for (size_t i = 0; i < NUM_GRN_TILES; i++) {
    int x = (rand() % width) + 1;
    int y = (rand() % height) + 1;
    mazeTile[x][y] = GREEN;
    API_setColor(x, y, 'g');
  }
  for (size_t i = 0; i < NUM_BLU_TILES; i++) {
    int x = (rand() % width) + 1;
    int y = (rand() % height) + 1;
    mazeTile[x][y] = BLUE;
    API_setColor(x, y, 'b');
  }
  for (size_t i = 0; i < NUM_YEL_TILES; i++) {
    int x = (rand() % width) + 1;
    int y = (rand() % height) + 1;
    mazeTile[x][y] = YELLOW;
    API_setColor(x, y, 'y');
  }
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

char *makeDebugStr(int x, int y, char color, char heading, int surviors,
                   int caches, int hazards) {
  char *str = (char *)malloc(150 * sizeof(char));
  if (str != NULL) {
    sprintf(str,
            "Heading: %c, x:%i, y:%i, color:%c\n surviors found: %i/3, caches "
            "found: %i/5 Hazards found: %i/3",
            heading, x, y, color, surviors, caches, hazards);
  }
  return str;
}

int main() {
  mazeInit(API_mazeWidth(), API_mazeHeight());
  while (1) {
    Info info = getInfo();
    Action decsion = decide(API_wallFront(), mazeTile[info.x][info.y]);
    plotWalls(info.x, info.y, info.cell);
    switch (decsion) {
    case FORWARD:
      API_setColor(info.x, info.y, info.color);
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
    default:
      debug_log(const_cast<char *>("undefined state"));
      break;
    }
    char *str =
        makeDebugStr(info.x, info.y, info.color, info.heading,
                     info.survivorsFound, info.cachesFound, info.hazardsFound);
    debug_log(str);
    free(str);
  }
}
