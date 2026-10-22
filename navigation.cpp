#include "navigation.h"
#include "./dataStructs/Queue.h"
#include "./dataStructs/Stack.h"
#include "GlobalDefs.h"
#include <string.h>

Cell maze[MAZE_HEIGHT][MAZE_WIDTH];

int cachesFound = 0;
int survivorsFound = 0;
int hazardsFound = 0;
struct AssmeblyTile assmeblyTile;
struct AssmeblyTile startTile;

int x = MAZE_WIDTH / 2;
int y = MAZE_HEIGHT / 2;

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

void revertXY() {
  switch (heading) {
  case NORTH:
    y--;
    break;
  case EAST:
    x--;
    break;
  case SOUTH:
    y++;
    break;
  case WEST:
    x++;
    break;
  }
}

void updateWall(Cell *cell, bool wallFront) {
  Wall temp;
  if (wallFront)
    temp = TRUE;
  else
    temp = FALSE;
  switch (heading) {
  case NORTH:
    cell->wallNorth = temp;
    if (inBounds(x, y + 1))
      maze[x][y + 1].wallSouth = temp;
    break;
  case EAST:
    cell->wallEast = temp;
    if (inBounds(x + 1, y))
      maze[x + 1][y].wallWest = temp;
    break;
  case SOUTH:
    cell->wallSouth = temp;
    if (inBounds(x, y - 1))
      maze[x][y - 1].wallNorth = temp;
    break;
  case WEST:
    cell->wallWest = temp;
    if (inBounds(x - 1, y))
      maze[x - 1][y].wallEast = temp;
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
  case REVERSE:
  case FLIP:
    heading = static_cast<Heading>((heading + 2) % 4);
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
        if (maze[i][j].x == 0)
          maze[i][j].wallWest = TRUE;
        if (maze[i][j].x == MAZE_WIDTH)
          maze[i][j].wallEast = TRUE;
        if (maze[i][j].y == 0)
          maze[i][j].wallSouth = TRUE;
        if (maze[i][j].y == MAZE_HEIGHT)
          maze[i][j].wallNorth = TRUE;
      }
    }
    push(&edgeNodes, &maze[MAZE_WIDTH / 2][MAZE_HEIGHT / 2]);
    push(&visitedNodes, &maze[MAZE_WIDTH / 2][MAZE_HEIGHT / 2]);
    run = true;
  }
}


void clearPlan() {
  while (!isEmpty(&actions))
    dequeue(&actions);
  if (visitedNodes.size > 0)
    pop(&visitedNodes);
}

Wall wallAt(Cell *cell, Heading heading) {
  switch (heading) {
  case NORTH:
    return cell->wallNorth;
  case EAST:
    return cell->wallEast;
  case SOUTH:
    return cell->wallSouth;
  default:
    return cell->wallWest;
  }
}

static Cell *findNeighbor(Cell *cell, Heading heading) {
  switch (heading) {
  case NORTH:
    return ((cell->wallNorth != TRUE) && inBounds(cell->x, cell->y + 1))
               ? &maze[cell->x][cell->y + 1]
               : nullptr;
  case EAST:
    return ((cell->wallEast != TRUE) && inBounds(cell->x + 1, cell->y))
               ? &maze[cell->x + 1][cell->y]
               : nullptr;
  case SOUTH:
    return ((cell->wallSouth != TRUE) && inBounds(cell->x, cell->y - 1))
               ? &maze[cell->x][cell->y - 1]
               : nullptr;
  case WEST:
    return ((cell->wallWest != TRUE) && inBounds(cell->x - 1, cell->y))
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
    int diff = (need - sim + 4) % 4; // 1=right, 2=flip, 3=left
    if (diff == 3)
      enqueue(&actions, &actionsMap[0]); // LEFT
    else
      for (int k = 0; k < diff; k++)
        enqueue(&actions, &actionsMap[1]);
    sim = need;
    enqueue(&actions, &actionsMap[2]);
  }
}

Action mapCell(bool wallFront, TileColor color, Cell *cell) {
  Tile type = classifyTile(color);

  if (type == TILE_HAZARD) {
    cell->type = TILE_HAZARD;
    hazardsFound++;
    updateHeading(REVERSE);
    updateXY();
    return REVERSE;
  }

  updateWall(cell, wallFront);

  Heading front = heading;
  Heading right = static_cast<Heading>((heading + 1) % 4);
  Heading behind = static_cast<Heading>((heading + 2) % 4);
  Heading left = static_cast<Heading>((heading + 3) % 4);

  // Check Right for unknown walls/paths
  if (wallAt(cell, right) == UNKNOWN) {
    updateHeading(RIGHT);
    return RIGHT;
  }

  // Check Left for unknown walls/paths
  if (wallAt(cell, left) == UNKNOWN) {
    updateHeading(LEFT);
    return LEFT;
  }

  // Check Behind (if unknown, perform a U-turn / REVERSE)
  if (wallAt(cell, behind) == UNKNOWN) {
    updateHeading(FLIP);
    return FLIP;
  }

  // 4. If all local directions are known, log tile type and continue exploring
  cell->type = type;
  switch (type) {
  case TILE_SURVIVOR:
    survivorsFound++;
    break;
  case TILE_CACHE:
    cachesFound++;
    break;
  case TILE_ASSEMBLY:
    assmeblyTile.found = true;
    assmeblyTile.cell = cell;
    break;
  case TILE_START:
    startTile.found = true;
    startTile.cell = cell;
  default:
    break;
  }
  return IDLE;
}

Cell *exploring(Cell *currentCell) {
  for (int i = 0; i < 4; i++) {
    Cell *childCell = findNeighbor(currentCell, static_cast<Heading>(i));
    if (childCell && !isVisited(childCell)) {
      push(&visitedNodes, childCell);
      push(&edgeNodes, childCell);
    }
  }

  Cell *target = nullptr;
  while (edgeNodes.size > 0) {
    Cell *possibleCell = static_cast<Cell *>(pop(&edgeNodes));
    if (possibleCell != currentCell && possibleCell->type != TILE_HAZARD) {
      target = possibleCell;
      break;
    }
  }
  return target; // always returns now
}

Action decide(bool wallFront, TileColor color) {
  Cell *currentCell = &maze[x][y];
  static bool pickedUp = false;
  init();

  //------Maping-------
  if (currentCell->type == TILE_UNKNOWN) {
    Action mapAction = mapCell(wallFront, color, currentCell);
    if (mapAction != IDLE) // still spinning to look at the walls
      return mapAction;
  }

  if (isEmpty(&actions)) {
    Cell *targetCell = nullptr;

    if (assmeblyTile.found && survivorsFound == NUM_SURVIVORS &&
        cachesFound == NUM_CACHE && startTile.found) {
      if (currentCell == assmeblyTile.cell)
        pickedUp = true;
      if (pickedUp) {
        if (currentCell == startTile.cell)
          return IDLE; // win condtion
        targetCell = startTile.cell;
      } else {
        targetCell = assmeblyTile.cell;
      }
    } else {
      targetCell = exploring(currentCell);
    }
    if (!targetCell)
      return IDLE; // nothing left to explore
    planPath(currentCell, targetCell);
  }

  Action finalAction = *static_cast<Action *>(dequeue(&actions));
  updateHeading(finalAction);
  if (finalAction == FORWARD)
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

Heading getHeading() { return heading; }

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
