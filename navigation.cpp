#include "navigation.h"
#include "./dataStructs/Queue.h"
#include "./dataStructs/Stack.h"

#define MAZE_HEIGHT 16
#define MAZE_WIDTH 16

Cell maze[MAZE_HEIGHT][MAZE_WIDTH];

int cachesFound = 0;
int survivorsFound = 0;
int hazardsFound = 0;

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
    maze[x][y + 1].wallSouth = wallFront;
    break;
  case EAST:
    cell->wallEast = wallFront;
    maze[x + 1][y].wallWest = wallFront;
    break;
  case SOUTH:
    cell->wallSouth = wallFront;
    maze[x][y - 1].wallNorth = wallFront;
    break;
  case WEST:
    cell->wallWest = wallFront;
    maze[x - 1][y].wallEast = wallFront;
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

static Stack edgeNodes; // next nodes to explore
static Stack visitedNodes;
static Queue actions;

void init() {
  static bool run = false;
  if (!run) {
    push(&edgeNodes, &maze[0][0]);
    push(&visitedNodes, &maze[0][0]);
    run = true;
  }
}

void mapCell(bool wallFront, TileColor color, Cell *cell) {
  static int loop = 0;
  loop = (loop + 1) % 4;
  if ((loop >= 3) && (cell->type == TILE_UNKNOWN)) {
    cell->x = x;
    cell->y = y;
    updateWall(cell, wallFront);
    updateHeading(RIGHT);
    return RIGHT;
  }
  cell->type = classifyTile(color);
}

Action decide(bool wallFront, TileColor color) {
  Cell *currentCell = &maze[x][y];
  Cell *nextCell;
  // TODO implement navigation
  init();
  if (currentCell->type == TILE_UNKNOWN) {
    mapCell(wallFront, color, currentCell);
  }

  for (int i = 0; i < 4; i++) {
    Heading testHeading = static_cast<Heading>(i);
    Cell *childCell;
    // tests if neighboring cells are accssable
    switch (testHeading) {
    case NORTH:
      if (!currentCell->wallNorth)
        childCell = &maze[x + 1][y];
      else
        continue;
      break;
    case EAST:
      if (!currentCell->wallEast)
        childCell = &maze[x][y + 1];
      else
        continue;
      break;
    case SOUTH:
      if (!currentCell->wallSouth)
        childCell = &maze[x - 1][y];
      else
        continue;
      break;
    case WEST:
      if (!currentCell->wallWest)
        childCell = &maze[x][y - 1];
      else
        continue;
      break;
    }
    // IF childCell is NOT in visitedNodes OR is a hazard
    // push childCell -> visitedNodes & edgeNodes
    for (int i = 0; i < visitedNodes.size; i++) {
      if ((childCell == visitedNodes.items[i]) ||
          (childCell->type == TILE_HAZARD))
        continue;
      push(&visitedNodes, childCell);
      push(&edgeNodes, childCell);
    }
    nextCell = static_cast<Cell *>(pop(&edgeNodes));
    // TODO find best path from current currentCell to next cell
  }
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
  info.survivorsFound = survivorsFound;
  info.cachesFound = cachesFound;
  info.hazardsFound = hazardsFound;
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
    info.color = 'c';
  }
  info.cell = maze[x][y];
  return info;
}
