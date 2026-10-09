#ifndef COLORCALIBRATION_H
#define COLORCALIBRATION_H

#include "navigation.h"
#include "vex.h"

using namespace vex;

#define NUM_SAMPLES 10
#define WAIT_TIME 200
// Fallback black threshold, only used if calibration is skipped
#define BRIGHT_LOW 10
#define BLACK_MARGIN 5      // headroom above the calibrated black level
#define BLACK_THRESH_MIN 2  // clamps, so one bad placement can neither swallow
#define BLACK_THRESH_MAX 40 // the whole course into BLACK nor lose the tile

#define COLOR_RED CAL_RED
#define COLOR_GREEN CAL_GREEN
#define COLOR_BLUE CAL_BLUE
#define COLOR_YELLOW CAL_YELLOW
#define COLOR_WHITE CAL_WHITE
#define COLOR_BLACK CAL_BLACK
#define COLOR_CALIBRATED_COUNT CAL_CALIBRATED_COUNT
#define COLOR_COUNT CAL_COUNT

#define PI 3.14159265358979 // VEXcode often lacks M_PI

enum CalColor {
  CAL_RED = 0,
  CAL_GREEN,
  CAL_BLUE,
  CAL_YELLOW,
  CAL_WHITE,
  CAL_BLACK,
  CAL_CALIBRATED_COUNT = CAL_BLACK, // colors that get a hue band
  CAL_COUNT = CAL_CALIBRATED_COUNT + 1
};

struct HueCalibration {
  double center;    // mean hue of the tile, kept in [0, 360)
  double tolerance; // half-width of the accepted band, in degrees
};

// Hardware and shared state (defined in ColorCalibration.cpp)
extern vex::brain Brain;
extern touchled touchLEDSensor;
extern optical opticalSensor;
extern HueCalibration gCal[COLOR_COUNT];
extern vex::touchled touchLEDSensor;
extern vex::optical opticalSensor;
extern vex::bumper bumpSensor;

// Shared Maze functions
void handleColorCal(void);
TileColor classifyTileColor(double hue, double bright);

// Hue math
double wrapHue(double h);
double hueDistance(double a, double b);
double circularMean(const double vals[], int n);

// Calibration
HueCalibration calibrateTileHue(CalColor c);
HueCalibration calibrateBlackBrightness(void);
double blackThreshold();

// Name / color lookups
TileColor indexToTileColor(CalColor c);
const char *tileColorName(TileColor c);
const char *colorName(CalColor c);
const char *sensorColorName(vex::color c);
color calLedColor(CalColor c);

#endif // COLORCALIBRATION_H
