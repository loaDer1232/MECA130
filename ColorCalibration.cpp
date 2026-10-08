#ifndef COLORCALIBRATION
#define COLORCALIBRATION
#include "ColorCalibration.h"
#include "navigation.h"
#include "vex.h"

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

using namespace vex;

vex::brain Brain;
touchled touchLEDSensor = touchled(PORT10);
optical opticalSensor = optical(PORT1);

TileColor indexToTileColor(CalColor c) {
  switch (c) {
  case COLOR_RED:
    return RED;
  case COLOR_GREEN:
    return GREEN;
  case COLOR_BLUE:
    return BLUE;
  case COLOR_YELLOW:
    return YELLOW;
  case COLOR_WHITE:
    return WHITE;
  case COLOR_BLACK:
    return BLACK;
  default:
    return WHITE; // unreachable
  }
}

HueCalibration gCal[COLOR_COUNT];

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

static const char *tileColorName(TileColor c) {
  switch (c) {
  case RED:
    return "RED";
  case GREEN:
    return "GREEN";
  case BLUE:
    return "BLUE";
  case YELLOW:
    return "YELLOW";
  case WHITE:
    return "WHITE";
  case BLACK:
    return "BLACK";
  default:
    return "?";
  }
}

static color calLedColor(CalColor c) {
  switch (c) {
  case COLOR_RED:
    return vex::red;
  case COLOR_GREEN:
    return vex::green;
  case COLOR_BLUE:
    return vex::blue;
  case COLOR_YELLOW:
    return vex::yellow;
  case COLOR_WHITE:
    return vex::white;
  case COLOR_BLACK:
    return vex::black;
  default:
    return vex::white;
  }
}

// Black has no hue band, so it is decided purely on brightness. The threshold
// comes from the black calibration step; BRIGHT_LOW is the fallback for when
// that step was skipped.
static double blackThreshold() {
  return (gCal[COLOR_BLACK].tolerance > 0.0) ? gCal[COLOR_BLACK].center
                                             : (double)BRIGHT_LOW;
}

HueCalibration calibrateTileHue(color c) {
  (void)c; // the band is found from the samples, not assumed from the name
  double samples[NUM_SAMPLES];
  for (int i = 0; i < NUM_SAMPLES; i++) {
    samples[i] = opticalSensor.hue();
  }

  double center = circularMean(samples, NUM_SAMPLES);

  // Tolerance is the widest spread seen, plus a margin for lighting drift,
  // floored so noise cannot shrink it and capped so bands cannot merge.
  double spread = 0.0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    double d = hueDistance(samples[i], center);
    if (d > spread)
      spread = d;
  }
  double tolerance = spread + 10.0;
  if (tolerance < 10.0)
    tolerance = 10.0;
  if (tolerance > 45.0)
    tolerance = 45.0;

  return HueCalibration{center, tolerance};
}

// The black equivalent: average the brightness the same way, then add a margin
// so lighting drift cannot push the real black tile back over the line. The
// clamp is what stops a misplaced sample from setting a threshold that swallows
// the whole course as BLACK.
HueCalibration calibrateBlackBrightness(void) {
  double sum = 0.0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += opticalSensor.brightness();
  }
  double threshold = sum / (double)NUM_SAMPLES + BLACK_MARGIN;
  if (threshold < BLACK_THRESH_MIN)
    threshold = BLACK_THRESH_MIN;
  if (threshold > BLACK_THRESH_MAX)
    threshold = BLACK_THRESH_MAX;
  return HueCalibration{threshold, BLACK_MARGIN};
}

static const char *colorName(CalColor c) {
  switch (c) {
  case COLOR_RED:
    return "RED";
  case COLOR_GREEN:
    return "GREEN";
  case COLOR_BLUE:
    return "BLUE";
  case COLOR_YELLOW:
    return "YELLOW";
  case COLOR_WHITE:
    return "WHITE";
  case COLOR_BLACK:
    return "BLACK";
  default:
    return "WHITE";
  }
}

static const char *sensorColorName(vex::color c) {
  if (c == vex::red)
    return "RED";
  if (c == vex::green)
    return "GREEN";
  if (c == vex::blue)
    return "BLUE";
  if (c == vex::yellow)
    return "YELLOW";
  if (c == vex::white)
    return "WHITE";
  if (c == vex::black)
    return "BLACK";
  if (c == vex::purple)
    return "PURPLE";
  if (c == vex::orange)
    return "ORANGE";
  return "BLACK";
}

#endif