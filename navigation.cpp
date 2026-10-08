#include "navigation.h"
#include "./dataStructs/Queue.h"
#include "./dataStructs/Stack.h"
#include <string.h>

#define MAZE_HEIGHT 16
#define MAZE_WIDTH 16

Cell maze[MAZE_HEIGHT][MAZE_WIDTH];

int cachesFound = 0;
int survivorsFound = 0;
int hazardsFound = 0;
bool assmbleyFound = false;

int x = 0;
int y = 0;

Heading heading = NORTH;

// checks if cell inbounds
static bool inBounds(int cellX, int cellY) {
  return cellX >= 0 && cellX < MAZE_WIDTH && cellY >= 0 && cellY < MAZE_HEIGHT;
}

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
    if (inBounds(x, y + 1)) // [FIX 9]
      maze[x][y + 1].wallSouth = wallFront;
    break;
  case EAST:
    cell->wallEast = wallFront;
    if (inBounds(x + 1, y)) // [FIX 9]
      maze[x + 1][y].wallWest = wallFront;
    break;
  case SOUTH:
    cell->wallSouth = wallFront;
    if (inBounds(x, y - 1)) // [FIX 9]
      maze[x][y - 1].wallNorth = wallFront;
    break;
  case WEST:
    cell->wallWest = wallFront;
    if (inBounds(x - 1, y)) // [FIX 9]
      maze[x - 1][y].wallEast = wallFront;
    break;
  }
}

void updateHeading(Action direction) {
  switch (direction) {
  case LEFT:
    heading = static_cast<Heading>((heading + 3) % 4);
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
static Cell *targetCell = nullptr;

void init() {
  static bool run = false;
  if (!run) {
    for (int i = 0; i < MAZE_WIDTH; i++) {
      for (int j = 0; j < MAZE_HEIGHT; j++) {
        maze[i][j].x = i;
        maze[i][j].y = j;
      }
    }
    push(&edgeNodes, &maze[0][0]);
    push(&visitedNodes, &maze[0][0]);
    run = true;
  }
}

static Cell *findNeighbor(Cell *cell, Heading heading) {
  switch (heading) {
  case NORTH:
    return (!cell->wallNorth && inBounds(cell->x, cell->y + 1))
               ? &maze[cell->x][cell->y + 1]
               : nullptr;
  case EAST:
    return (!cell->wallEast && inBounds(cell->x + 1, cell->y))
               ? &maze[cell->x + 1][cell->y]
               : nullptr;
  case SOUTH:
    return (!cell->wallSouth && inBounds(cell->x, cell->y - 1))
               ? &maze[cell->x][cell->y - 1]
               : nullptr;
  case WEST:
    return (!cell->wallWest && inBounds(cell->x - 1, cell->y))
               ? &maze[cell->x - 1][cell->y]
               : nullptr;
  }
  return nullptr;
}

bool isVisited(Cell *cell) {
  for (int i = 0; i < visitedNodes.size; i++) {
    if (cell == visitedNodes.items[i])
      return true;
  }
  return false;
}

Heading headingBetween(Cell *cell1, Cell *cell2) {
  if (cell1->y > cell2->y) {
    return SOUTH;
  }
  if (cell1->y < cell2->y) {
    return NORTH;
  }
  if (cell1->x > cell2->x) {
    return WEST;
  }
  if (cell1->x < cell2->x) {
    return EAST;
  }
  return NORTH;
}

static void planPath(Cell *from, Cell *to) {
  static Cell *parent[MAZE_HEIGHT][MAZE_WIDTH];
  static bool seen[MAZE_HEIGHT][MAZE_WIDTH];
  static Action actionsMap[] = {LEFT, RIGHT, FORWARD};
  memset(seen, 0, sizeof seen);

  // simple array queue for BFS
  Cell *q[MAZE_HEIGHT * MAZE_WIDTH];
  int head = 0, tail = 0;
  q[tail++] = from;
  seen[from->x][from->y] = true;
  parent[from->x][from->y] = nullptr;

  while (head < tail && !seen[to->x][to->y]) {
    Cell *c = q[head++];
    for (int h = 0; h < 4; h++) {
      Cell *n = findNeighbor(c, static_cast<Heading>(h));
      if (!n || seen[n->x][n->y])
        continue;
      // only walk through mapped, non-hazard cells (target may be unmapped)
      if (n != to && (n->type == TILE_UNKNOWN || n->type == TILE_HAZARD))
        continue;
      seen[n->x][n->y] = true;
      parent[n->x][n->y] = c;
      q[tail++] = n;
    }
  }
  if (!seen[to->x][to->y])
    return; // unreachable

  // walk back from target, then replay forwards
  Cell *path[MAZE_HEIGHT * MAZE_WIDTH];
  int len = 0;
  for (Cell *c = to; c; c = parent[c->x][c->y])
    path[len++] = c;

  Heading sim = heading; // simulated, don't touch the real one
  for (int i = len - 1; i > 0; i--) {
    Heading need = headingBetween(path[i], path[i - 1]);
    while (sim !=
           need) { // turn right until aligned (or add LEFT for shorter turns)
      enqueue(&actions, &actionsMap[1]);
      sim = static_cast<Heading>((sim + 1) % 4);
    }
    enqueue(&actions, &actionsMap[2]);
  }
}

Action mapCell(bool wallFront, TileColor color, Cell *cell) {
  static int loop = 0;
  if (loop < 4) {
    updateWall(cell, wallFront);
    updateHeading(RIGHT);
    loop++;
    return RIGHT;
  }
  loop = 0;
  cell->type = classifyTile(color);
  if (cell->type != TILE_NORMAL) {
    switch (cell->type) {
    case TILE_HAZARD:
      hazardsFound++;
      break;
    case TILE_SURVIVOR:
      survivorsFound++;
      break;
    case TILE_CACHE:
      cachesFound++;
      break;
    case TILE_ASSEMBLY:
      assmbleyFound = true;
      break;
    default:
      break;
    }
  }
  return IDLE; // "mapping finished"; decide() carries on planning (see [FIX 2])
}

Action decide(bool wallFront, TileColor color) {
  Cell *currentCell = &maze[x][y];
  init();
  if (currentCell->type == TILE_UNKNOWN) {
    Action mapAction = mapCell(wallFront, color, currentCell);
    if (mapAction != IDLE) // still spinning to look at the walls
      return mapAction;
    // returning IDLE. IDLE from decide() now only ever means "finished".
  }

  // finds valid edgeNodes
  for (int i = 0; i < 4; i++) {
    Heading testHeading = static_cast<Heading>(i);
    Cell *childCell = findNeighbor(currentCell, testHeading);
    // IF childCell is NOT in visitedNodes
    // push childCell -> visitedNodes & edgeNodes
    if (childCell && !isVisited(childCell)) {
      push(&visitedNodes, childCell);
      push(&edgeNodes, childCell);
    }
  }

  // only choose a new target once the previous plan has finished
  while (isEmpty(&actions)) {
    targetCell = nullptr;
    while (edgeNodes.size > 0) {
      Cell *possibleCell = static_cast<Cell *>(pop(&edgeNodes));
      if (possibleCell != currentCell && possibleCell->type != TILE_HAZARD) {
        targetCell = possibleCell;
        break;
      }
    }
    if (!targetCell) // no more cells to explore
      return IDLE;

    // Navigate from current to target cell
    planPath(currentCell, targetCell);
  }

  Action finalAction = *static_cast<Action *>(dequeue(&actions));
  updateHeading(finalAction);
  if (finalAction == FORWARD) // turning on the spot must not move x/y
    updateXY();
  return finalAction;
}

Tile classifyTile(TileColor color) {
  switch (color) {
  case BLACK:
    return TILE_NORMAL;
  case WHITE:
    return TILE_START;
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

Heading getHeading(){
  return heading;
}

Info getInfo() {
  Info info;
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
  case WEST:
    info.heading = 'w';
    break;
  case SOUTH:
    info.heading = 's';
    break;
  }

  switch (maze[x][y].type) {
  case TILE_NORMAL:
    info.color = 'k';
    break;
  case TILE_START:
    info.color = 'w';
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
  info.target = targetCell ? *targetCell : maze[x][y];
  return info;
}
