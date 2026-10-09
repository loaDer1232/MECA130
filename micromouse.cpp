// #ifdef Micromouse
#include "./API/API.h"
#include "navigation.h"
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>

#define NUM_RED_TILES 3
#define NUM_YEL_TILES 4
#define NUM_BLU_TILES 3
#define NUM_GRN_TILES 1

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
            "found: %i/3, caches "
            "found: %i/5 Hazards found: %i/3",
            heading, x, y, targetX, targetY, color, surviors, caches, hazards);
  }
  return str;
}

int main() {
  typedef struct {
    double center;    // mean hue of the tile, kept in [0, 360)
    double tolerance; // half-width of the accepted band, in degrees
  } HueCalibration;

  HueCalibration gCal[COLOUR_COUNT];

  static constexpr double PI = 3.14159265358979; // VEXcode often lacks M_PI

  double wrapHue(double h) { // hue is cyclic, so fold any angle into [0, 360)
    h = fmod(h, 360.0);
    if (h < 0.0)
      h += 360.0;
    return h;
  }

  // Shortest angular distance between two hues, always in [0, 180]. This is the
  // wraparound case: 355 and 5 are 350 apart the long way, so the answer is 10.
  double hueDistance(double a, double b) {
    double d = fabs(wrapHue(a) - wrapHue(b));
    return d > 180.0 ? 360.0 - d : d;
  }

  // Average hues as vectors on a unit circle. A plain mean of {356, 358, 2, 4}
  // returns 180 (green) because the 0/360 seam splits the samples.
  double circularMean(const double vals[], int n) {
    double sX = 0.0, sY = 0.0;
    for (int i = 0; i < n; i++) {
      double rad = wrapHue(vals[i]) * PI / 180.0;
      sX += cos(rad);
      sY += sin(rad);
    }
    return wrapHue(atan2(sY, sX) * 180.0 / PI);
  }

  TileColor indexToTileColor(Colour c) {
    switch (c) {
    case COLOUR_RED:
      return RED;
    case COLOUR_GREEN:
      return GREEN;
    case COLOUR_BLUE:
      return BLUE;
    case COLOUR_YELLOW:
      return YELLOW;
    case COLOUR_WHITE:
      return WHITE;
    case COLOUR_BLACK:
      return BLACK;
    default:
      return WHITE; // unreachable
    }
  }

  // Black has no hue band, so it is decided purely on brightness. The threshold
  // comes from the black calibration step; BRIGHT_LOW is the fallback for when
  // that step was skipped.
  static double blackThreshold() {
    return (gCal[COLOUR_BLACK].tolerance > 0.0) ? gCal[COLOUR_BLACK].center
                                                : (double)BRIGHT_LOW;
  }

  // Nearest calibrated centre, accepted only if it is inside that colour's
  // band. No hue colour is special-cased, so no hand-written hue range can be
  // wrong.
  TileColor classifyTileColor(double hue, double bright) {
    if (bright < blackThreshold()) {
      return BLACK; // hue is unreliable on dark surfaces
    }
    int bestIdx = 0;
    double bestDist = hueDistance(hue, gCal[COLOUR_RED].center);
    for (int i = 1; i < COLOUR_CALIBRATED_COUNT; i++) {
      double d = hueDistance(hue, gCal[i].center);
      if (d < bestDist) {
        bestDist = d;
        bestIdx = i;
      }
    }

    if (bestDist <= gCal[bestIdx].tolerance) {
      return indexToTileColor((Colour)bestIdx);
    }
    // Outside every calibrated band. Black is already caught above, so white
    // (the calibrated finish tile) is the only sensible fallback.
    return WHITE;
  }

  using namespace vex;

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
