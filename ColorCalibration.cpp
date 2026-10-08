#include "vex.h"
#include "navigation.h"
#include "ColorCalibration.h"
#include <cmath>

// vex::brain Brain;                            commented out b/c multiple definitions
// touchled touchLEDSensor = touchled(PORT10);
// optical opticalSensor = optical(PORT1);

HueCalibration gCal[COLOR_COUNT];

// ---------------------------------------------------------------------------
// Hue math
// ---------------------------------------------------------------------------

// Hue is cyclic, so fold any angle into [0, 360)
double wrapHue(double h) {
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

// ---------------------------------------------------------------------------
// Calibration
// ---------------------------------------------------------------------------

HueCalibration calibrateTileHue(CalColor c) {
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

// Black has no hue band, so it is decided purely on brightness. The threshold
// comes from the black calibration step; BRIGHT_LOW is the fallback for when
// that step was skipped.
double blackThreshold() {
  return (gCal[COLOR_BLACK].tolerance > 0.0) ? gCal[COLOR_BLACK].center
                                             : (double)BRIGHT_LOW;
}

// ---------------------------------------------------------------------------
// Name / color lookups
// ---------------------------------------------------------------------------

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

const char *tileColorName(TileColor c) {
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

const char *colorName(CalColor c) {
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

const char *sensorColorName(vex::color c) {
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

color calLedColor(CalColor c) {
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

// Nearest calibrated centre, accepted only if it is inside that color's band.
// No hue color is special-cased, so no hand-written hue range can be wrong.
TileColor classifyTileColor(double hue, double bright) {
  if (bright < blackThreshold()) {
    return BLACK; // hue is unreliable on dark surfaces
  }
  int bestIdx = 0;
  double bestDist = hueDistance(hue, gCal[COLOR_RED].center);
  for (int i = 1; i < COLOR_CALIBRATED_COUNT; i++) {
    double d = hueDistance(hue, gCal[i].center);
    if (d < bestDist) {
      bestDist = d;
      bestIdx = i;
    }
  }

  if (bestDist <= gCal[bestIdx].tolerance) {
    return indexToTileColor((CalColor)bestIdx);
  }
  // Outside every calibrated band. Black is already caught above, so white
  // (the calibrated finish tile) is the only sensible fallback.
  return WHITE;
}

static const CalColor gCalOrder[] = {COLOR_BLACK, COLOR_RED,    COLOR_GREEN,                //array for color order of initial calibration
                                     COLOR_BLUE,  COLOR_YELLOW, COLOR_WHITE};
static const int CAL_STEPS = (int)(sizeof(gCalOrder) / sizeof(gCalOrder[0]));


// go through each tile once, save its custom hue/brightness range, and skip any
// tile that should be left at the sensor's default value. The wrapped hue logic
// is retained so red's 0/360 seam does not split the calibration band.
void handleColorCal(void) {
  for (int i = 0; i < COLOR_COUNT; i++) {
    gCal[i].center = 0.0;
    gCal[i].tolerance = 0.0;
  }

  for (int step = 0; step < CAL_STEPS; step++) {
    CalColor target = gCalOrder[step];
    bool done = false;
    const char *status = "CHECK";

    // Bumper held down at the first step: skip the whole calibration.
    if (target == COLOR_BLACK && bumpSensor.pressing()) {
      Brain.Screen.clearScreen();
      Brain.Screen.setCursor(1, 1);
      Brain.Screen.print("Skip calibration");
      gState = STATE_IDLE;
      break;
    }

    touchLEDSensor.on(calLedColor(target));

    while (!done) {
      double hue = opticalSensor.hue();
      double bright = opticalSensor.brightness();
      TileColor predicted = classifyTileColor(hue, bright);

      Brain.Screen.clearScreen();
      Brain.Screen.setCursor(1, 1);
      Brain.Screen.print("CAL %s %s", colorName(target), status);
      Brain.Screen.setCursor(2, 1);
      Brain.Screen.print("H:%3d B:%3d", (int)hue, (int)bright);
      Brain.Screen.setCursor(3, 1);
      Brain.Screen.print("sensor=%s", sensorColorName(opticalSensor.color()));
      if (target == COLOR_BLACK) {                                                  //seperate msg for skipping calibration entirely
        Brain.Screen.setCursor(4, 1);
        Brain.Screen.print("LED=SAVE BUMP=SKIPCAL");
      }else {
      Brain.Screen.setCursor(4, 1);
      Brain.Screen.print("LED=SAVE  BUMP=SKIP");
      }
      Brain.Screen.setCursor(5, 1);
      Brain.Screen.print("Tile=%s", tileColorName(predicted));

      if (bumpSensor.pressing()) {
        status = "SKIP";
        done = true;
      } else if (touchLEDSensor.pressing()) {
        if (target == COLOR_BLACK) {
          gCal[target] = calibrateBlackBrightness();
        } else {
          gCal[target] = calibrateTileHue(target);
        }
        status = "SAVED";
        done = true;
      }

      wait(WAIT_TIME, msec);
    }

    Brain.Screen.clearScreen();
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("CAL %s %s", colorName(target), status);
    Brain.Screen.setCursor(4, 1);
    Brain.Screen.print("release to continue");
    touchLEDSensor.setBlink(calLedColor(target), 0.15, 0.15);
    wait(900, msec);
    touchLEDSensor.setBrightness(0);
  }

  Brain.Screen.clearScreen();
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Calibration done");
  Brain.Screen.setCursor(2, 1);
  Brain.Screen.print("On start tile, facing start");
  Brain.Screen.setCursor(3, 1);
  Brain.Screen.print("Press to explore");
  touchLEDSensor.on(white);

  while (!touchLEDSensor.pressing()) {
    wait(WAIT_TIME, msec);
  }
  touchLEDSensor.setBrightness(0);
  gState = STATE_INIT;
}
