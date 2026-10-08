/*----------------------------------------------------------------------------*/
/* */
/* Module: main.cpp */
/* Author: kinganseam erasmuchri */
/* Created: 03/09/2026, 11:00:00 */
/* Description: IQ2 project */
/* Final Project MECA130*/
/*----------------------------------------------------------------------------*/
#include "navigation.h"
#include "vex.h"
#include <cmath>

#define LOOP_DELAY 200

#define WALL_SET_MM 300
#define WALL_CLEAR_MM 320
#define WAIT_TIME 200
#define SCREEN_RESET_X 1
#define SCREEN_RESET_Y 1
#define NUM_SAMPLES 10

#define BUFFER_SIZE 1

#define QUARTER_TURN 90 // task 4
#define FULL_CIRCLE 360

#define GEAR_RATIO 1
#define WHEEL_DIAMETER_MM 63.6                            // task 1.1.1
#define WHEEL_CIRCUMFERENCE (3.14159 * WHEEL_DIAMETER_MM) // task 1.2
#define TRACK_WIDTH_MM 18.63                              // task 1.3
#define WHEEL_BASE_MM 121.7                               // task 1.3
#define CELL_SIZE_MM 300.0 // adjust to r/w measurement

#define INERTIAL_TOLERANCE 2
#define HEADING_NORTH 0
#define HEADING_EAST 90
#define HEADING_SOUTH 180
#define HEADING_WEST 270

#define BRIGHT_LOW 10        // fallback black threshold, only if calibration is skipped
#define BLACK_MARGIN 5       // headroom above the calibrated black level
#define BLACK_THRESH_MIN 2   // clamps, so one bad placement can neither swallow
#define BLACK_THRESH_MAX 40  // the whole course into BLACK nor lose the tile

#define COLOR_RED CAL_RED
#define COLOR_GREEN CAL_GREEN
#define COLOR_BLUE CAL_BLUE
#define COLOR_YELLOW CAL_YELLOW
#define COLOR_WHITE CAL_WHITE
#define COLOR_BLACK CAL_BLACK
#define COLOR_CALIBRATED_COUNT CAL_CALIBRATED_COUNT
#define COLOR_COUNT CAL_COUNT

typedef enum {
  CAL_RED = 0,
  CAL_GREEN,
  CAL_BLUE,
  CAL_YELLOW,
  CAL_WHITE,
  CAL_BLACK,
  CAL_CALIBRATED_COUNT = CAL_BLACK,
  CAL_COUNT = CAL_CALIBRATED_COUNT + 1
} CalColor;

typedef enum RobotState {
  STATE_INIT,
  STATE_COLOR_CAL,
  STATE_IDLE,
  STATE_COLOR_CHECK,
  STATE_WALL_CHECK,
  STATE_DECIDE,
  STATE_MOVE,
  STATE_ERROR,
} RobotState;

typedef struct HueCalibration {
  double center;    // mean hue of the tile, kept in [0, 360)
  double tolerance; // half-width of the accepted band, in degrees
} HueCalibration;

static constexpr double PI = 3.14159265358979; // VEXcode often lacks M_PI

TileColor indexToTileColor(CalColor c) {
  switch (c) {
  case COLOR_RED :
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
// Black has no hue band, so it is decided purely on brightness. The threshold
// comes from the black calibration step; BRIGHT_LOW is the fallback for when
// that step was skipped.
static double blackThreshold() {
  return (gCal[COLOR_BLACK].tolerance > 0.0) ? gCal[COLOR_BLACK].center
                                             : (double)BRIGHT_LOW;
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

using namespace vex;

typedef struct {
  double max;
  double min;
} HueValues;

HueValues redHue, greenHue, blueHue, yellowHue;

// TileColor classifyTileColor(double hue, double bright) {
//   // if (bright < BRIGHT_LOW) {
//   //   return BLACK;
//   // }

//   if (hue > greenHue.min && hue < greenHue.max) {
//     return GREEN;
//   }
//   if (hue > blueHue.min && hue < blueHue.max) {
//     return BLUE;
//   }
//   if (hue > yellowHue.min && hue < yellowHue.max) {
//     return YELLOW;
//   }
//   if (hue > redHue.min || hue < redHue.max) {
//     return RED;
//   }
//   return WHITE;
// }

using namespace vex;

vex::brain Brain;

motor leftMotor =
    motor(PORT6, false); // standard direction check port***   task 1.4
motor rightMotor = motor(PORT12, true);   // reversed (mirrored mounting)
inertial brainInertial = inertial(right); // task 1.4

smartdrive Drivetrain = smartdrive( // task 1.4
    leftMotor, rightMotor, brainInertial, WHEEL_CIRCUMFERENCE, TRACK_WIDTH_MM,
    WHEEL_BASE_MM, mm, GEAR_RATIO); // task 1.4

distance distanceSensor = distance(PORT5);
touchled touchLEDSensor = touchled(PORT10);
optical opticalSensor = optical(PORT1);

bumper bumpSensor = bumper(PORT11);

double buffer[BUFFER_SIZE];
int writeIndex = 0;
int count = 0;

void bufferWrite(double value) {
  buffer[writeIndex] = value;
  writeIndex = (writeIndex + 1) % BUFFER_SIZE;
  if (count < BUFFER_SIZE)
    count++; // task 3.1
}

double bufferAverage(void) {
  double total = 0.0;
  for (int i = 0; i < count; i++) {
    total += buffer[i];
  }
  return total / count;
}

bool updateWallDetection(double avg) {
  static bool wallDetected;
  if (wallDetected) {
    if (avg > WALL_CLEAR_MM)
      wallDetected = false;
  } else {
    if (avg < WALL_SET_MM)
      wallDetected = true;
  }
  return wallDetected;
}
static RobotState gState = STATE_COLOR_CAL;
static Heading gHeading = NORTH;
static int gNextDir = 0;
static bool gWallAhead = false;
TileColor gtileColor;

void hitWall(Action wall) {
  //   // recovery script for hitting wall
  //   directionType dir1, dir2;
  //   int Tries = 0;
  //   double reading = distanceSensor.objectDistance(mm);
  //   double prevReading = reading;

  //   switch (wall) {
  //   case LEFT:
  //     dir1 = left;
  //     dir2 = right;
  //     break;
  //   case RIGHT:
  //     dir1 = right;Brain.Screen.setCursor(1, 1);
  //     dir2 = left;
  //     break;
  //   default:
  //     gState = STATE_ERROR;
  //     return;
  //   }

  //   while (Tries < 5) {
  //     Drivetrain.Turnfor(dir1, 90, deg, 20,
  //                        percent, false); // slow turn to gather wall
  //                        measurements

  //   cali_wiggle:
  //     for (int i = 0; i < WALL_READINGS; i++) {
  //       prevReading = reading;
  //       reading = distanceSensor.objectDistance(mm);
  //       wait(10, msec);

  //       if (reading > prevReading) {
  //         Drivetra % 360.0in.Turnfor(dir2, 45, deg, 20, percent,
  //                            false); // approximate turn

  //         for (int j = 0; j < WALL_READINGS; j++) {
  //           prevReading = reading;
  //           reading = distanceSensor.objectDistance(mm);
  //           wait(10, msec);

  //           if (reading > prevReading + 5) {
  //             Tries++;
  //             if (Tries >= 5) {Brain.Screen.setCursor(1, 1);
  //               gState =
  //                   STATE_ERROR; // goes to error state if cannot
  //                   self-re-orient
  //               return;
  //             }
  //             goto cali_wiggle;
  //           } else {
  //             Drivetrain.Turnfor(dir2, 90, deg, 40, percent,
  //                                true); // reorientation hopefully complete
  //             touchLEDSensor.set_brightness(100);
  //             touchLEDSensor.setBlink(red, 1, 1);
  //             wait(400, msec);
  //             gState = STATE_COLOR_CHECK;
  //             return;
  //           }
  //         }
  //       }Brain.Screen.setCursor(1, 1);
  //     }
  //   }
  //   gState = STATE_ERROR; // goes to error state if cannot self-re-orient
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

static const color gCalOrder[] = {COLOR_BLACK, COLOR_RED,    COLOR_GREEN,
                                  COLOR_BLUE,  COLOR_YELLOW, COLOR_WHITE};
static const int CAL_STEPS = (int)(sizeof(gCalOrder) / sizeof(gCalOrder[0]));

static const char *colorName(color c) {
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
    return "?";
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

static color calLedColor(color c) {
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

// --- Handlers ---

void handleInit(void) {
  touchLEDSensor.on(purple);
  // Brain.Screen.clearScreen();
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Calibrating...");

  brainInertial.calibrate();
  do {
    wait(WAIT_TIME, msec);
  } while (brainInertial.isCalibrating());

  gHeading = NORTH;
  gState = STATE_IDLE;
}
// go through each tile once, save its custom hue/brightness range, and skip any
// tile that should be left at the sensor's default value. The wrapped hue logic
// is retained so red's 0/360 seam does not split the calibration band.
void handleColorCal(void) {
  for (int i = 0; i < COLOR_COUNT; i++) {
    gCal[i].center = 0.0;
    gCal[i].tolerance = 0.0;
  }

  for (int step = 0; step < CAL_STEPS; step++) {
    color target = gCalOrder[step];
    bool done = false;
    const char *status = "CHECK";

    // Bumper held down at the first step: skip the whole calibration.
    if (target == COLOR_BLACK && bumpSensor.pressing()) {
      Brain.Screen.clearScreen();
      Brain.Screen.setCursor(1, 1);
      Brain.Screen.print("Skip calibration");
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
      Brain.Screen.setCursor(4, 1);
      Brain.Screen.print("LED=SAVE  BUMP=SKIP");
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

// ---- Handlers ----
static bool resetButtonPressed(void) {
  return touchLEDSensor.pressing() || bumpSensor.pressing();
}

void handleIdle(void) {
  touchLEDSensor.on(white);
  Brain.Screen.clearScreen();
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Press to start");

  if (touchLEDSensor.pressing()) {
    gState = STATE_COLOR_CHECK;
  }
}

void handlecolorCheck(void) {
  touchLEDSensor.on(blue);
  gtileColor =
      classifyTileColor(opticalSensor.hue(), opticalSensor.brightness());

  Brain.Screen.clearScreen();
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Tile: %d", gtileColor);

  // EXTENSION POINT 2: objective handler states branch from here
  gState = STATE_WALL_CHECK;
}

void handleWallCheck(void) {
  touchLEDSensor.on(blue_green);

  // The robot is stationary here, so averaging is valid
  for (int i = 0; i < BUFFER_SIZE; i++) {
    bufferWrite(distanceSensor.objectDistance(mm));
    wait(WAIT_TIME, msec);
  }
  gWallAhead = updateWallDetection(bufferAverage());

  gState = STATE_DECIDE;
}

void handleDecide(void) {
  touchLEDSensor.on(yellow);
  Action decsion = decide(gWallAhead, gtileColor);
  switch (decsion) {
  case LEFT:
    gNextDir = static_cast<Heading>((getHeading()-1)%4);
    break;
  case RIGHT:
    gNextDir = static_cast<Heading>((getHeading()+1)%4);
    break;
  case IDLE:
    return;
  default:
    gNextDir = getHeading();
  }
  gState = STATE_MOVE;
}

void handleMove(void) {
  touchLEDSensor.on(green);

  if (gNextDir != getHeading()) {
    Drivetrain.turnToHeading((int)gNextDir * QUARTER_TURN, degrees);
  } else {
    gNextDir = getHeading();
    Drivetrain.driveFor(forward, CELL_SIZE_MM, mm);
  }
  // EXTENSION POINT 3: collision detection goes here
  gState = STATE_COLOR_CHECK;
}
void handleRecovery(void) { // recovery
  // Drivetrain.stop();

  // touchLEDSensor.setBlink(orange, 1, 1);
  // Brain.Screen.setCursor(1, 1);
  // Brain.Screen.print("Trying to recover");
  // do {
  //   Drivetrain.setDriveVelocity(50, percent);
  //   Drivetrain.driveFor(reverse, 150);
  //   double d1 = 0;                          // distance measurement 1
  //   double d2 = 0;                          // distance measurement 2
  //   d1 = distanceSensor.objectDistance(mm); // distance to wall1
  //   Drivetrain.turnFor(left, 45, deg, 30, percent);
  //   d2 = distanceSensor.objectDistance(mm); // distance to wall2
  //   if (d2 > d1) {
  //     hitWall(RIGHT); // recovery from wallhit
  //   } else {
  //     hitWall(LEFT);
  //   };
  //   wait(2000, msec);
  // }
}

void handleError(void) {
  Drivetrain.stop();

  touchLEDSensor.on(red);
  // I dont like that I am calling for the debug object I made
  Info info = getInfo();
  Brain.Screen.print("place on center of %i, %i faceing %c", info.cell.x, info.cell.y,
                     info.heading);
  while (!touchLEDSensor.pressing())
    wait(WAIT_TIME, msec);

  // Enters loop agian at correct XY
  gState = STATE_COLOR_CHECK;
}

// --- Dispatcher ---

void runStateMachine(void) {
  switch (gState) {
  case STATE_INIT:
    handleInit();
    break;
  case STATE_COLOR_CAL:
    handleColorCal();
    break;
  case STATE_IDLE:
    handleIdle();
    break;
  case STATE_COLOR_CHECK:
    handlecolorCheck();
    break;
  case STATE_WALL_CHECK:
    handleWallCheck();
    break;
  case STATE_DECIDE:
    handleDecide();
    break;
  case STATE_MOVE:
    handleMove();
    break;
  case STATE_ERROR:
    handleError();
    break;
  default:
    Drivetrain.stop();
    break;
  }
}

int main() {
  opticalSensor.setLight(ledState::on);
  while (1) {
    runStateMachine();
    wait(LOOP_DELAY, msec);
  }
}
