#include "vex.h"
#include "navigation.h"
using namespace vex;

#define NUM_SAMPLES 10

#define BRIGHT_LOW                                                             \
  10 // fallback black threshold, only if calibration is skipped
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

typedef enum CalColor {
  CAL_RED = 0,
  CAL_GREEN,
  CAL_BLUE,
  CAL_YELLOW,
  CAL_WHITE,
  CAL_BLACK,
  CAL_CALIBRATED_COUNT = CAL_BLACK,
  CAL_COUNT = CAL_CALIBRATED_COUNT + 1,
};

 static constexpr double PI =
    3.14159265358979; // VEXcode often lacks M_PI

typedef struct HueCalibration {
  double center;    // mean hue of the tile, kept in [0, 360)
  double tolerance; // half-width of the accepted band, in degrees
} HueCalibration;

typedef struct {
  double max;
  double min;
} HueValues;

static bool resetButtonPressed(void);

TileColor indexToTileColor(CalColor c);
double wrapHue(double h);
double hueDistance(double a, double b);
double circularMean(const double vals[], int n);
static const char *tileColorName(TileColor c);
static double blackThreshold();
HueCalibration calibrateTileHue(CalColor c);
HueCalibration calibrateBlackBrightness(void);
static const char *colorName(CalColor c);
static const char *sensorColorName(vex::color c);
static color calLedColor(CalColor c);