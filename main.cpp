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

#define PI 3.14159265358979323846

typedef enum RobotState {
  STATE_INIT,
  STATE_COLOR_CAL,
  STATE_IDLE,
  STATE_COLOUR_CHECK,
  STATE_WALL_CHECK,
  STATE_DECIDE,
  STATE_MOVE,
  STATE_ERROR,
  STATE_RECOVERY,
} RobotState;

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

bumper Bumper1 = bumper(PORT11);

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
static Heading gNextDir = NORTH;
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
  //             gState = STATE_COLOUR_CHECK;
  //             return;
  //           }
  //         }
  //       }Brain.Screen.setCursor(1, 1);
  //     }
  //   }
  //   gState = STATE_ERROR; // goes to error state if cannot self-re-orient
}
HueCalibration calibrateTileHue(Colour c) {
  (void)c;  // the band is found from the samples, not assumed from the name
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
    if (d > spread) spread = d;
  }
  double tolerance = spread + 10.0;
  if (tolerance < 10.0) tolerance = 10.0;
  if (tolerance > 45.0) tolerance = 45.0;

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
  if (threshold < BLACK_THRESH_MIN) threshold = BLACK_THRESH_MIN;
  if (threshold > BLACK_THRESH_MAX) threshold = BLACK_THRESH_MAX;
  return HueCalibration{threshold, BLACK_MARGIN};
}

static const Colour gCalOrder[] = {
    COLOUR_BLACK, COLOUR_RED,   COLOUR_GREEN,
    COLOUR_BLUE,  COLOUR_YELLOW, COLOUR_WHITE};
static const int CAL_STEPS = (int)(sizeof(gCalOrder) / sizeof(gCalOrder[0]));

static const char *colourName(Colour c) {
  switch (c) {
  case COLOUR_RED:    return "RED";
  case COLOUR_GREEN:  return "GREEN";
  case COLOUR_BLUE:   return "BLUE";
  case COLOUR_YELLOW: return "YELLOW";
  case COLOUR_WHITE:  return "WHITE";
  case COLOUR_BLACK:  return "BLACK";
  default:            return "?";
  }
}

static const char *sensorColorName(vex::color c) {
  if (c == vex::red) return "RED";
  if (c == vex::green) return "GREEN";
  if (c == vex::blue) return "BLUE";
  if (c == vex::yellow) return "YELLOW";
  if (c == vex::white) return "WHITE";
  if (c == vex::black) return "BLACK";
  if (c == vex::purple) return "PURPLE";
  if (c == vex::orange) return "ORANGE";
  return "BLACK";
}

static colorType calLedColour(Colour c) {
  switch (c) {
  case COLOUR_RED:    return vex::red;
  case COLOUR_GREEN:  return vex::green;
  case COLOUR_BLUE:   return vex::blue;
  case COLOUR_YELLOW: return vex::yellow;
  case COLOUR_WHITE:  return vex::white;
  case COLOUR_BLACK:  return vex::black;
  default:            return vex::white;
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
// go through each tile once, save its custom hue/brightness range, and skip any tile
// that should be left at the sensor's default value. The wrapped hue logic is
// retained so red's 0/360 seam does not split the calibration band. 
void handleColorCal(void) {
  for (int i = 0; i < COLOUR_COUNT; i++) {
    gCal[i].center = 0.0;
    gCal[i].tolerance = 0.0;
  }

  for (int step = 0; step < CAL_STEPS; step++) {
    Colour target = gCalOrder[step];
    bool done = false;
    const char *status = "CHECK";

    // Bumper held down at the first step: skip the whole calibration.
    if (target == COLOUR_BLACK && bumpSensor.pressing()) {
      Brain.Screen.clearScreen();
      Brain.Screen.setCursor(1, 1);
      Brain.Screen.print("Skip calibration");
      break;
    }

    touchLEDSensor.on(calLedColour(target));

    while (!done) {
      double hue = opticalSensor.hue();
      double bright = opticalSensor.brightness();
      TileColor predicted = classifyTileColor(hue, bright);

      Brain.Screen.clearScreen();
      Brain.Screen.setCursor(1, 1);
      Brain.Screen.print("CAL %s %s", colourName(target), status);
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
        if (target == COLOUR_BLACK) {
          gCal[target] = calibrateBlackBrightness();
        } else {
          gCal[target] = calibrateTileHue(target);
        }
        status = "SAVED";
        done = true;
      }

      wait(WAIT_TIME_MS, msec);
    }

    Brain.Screen.clearScreen();
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("CAL %s %s", colourName(target), status);
    Brain.Screen.setCursor(4, 1);
    Brain.Screen.print("release to continue");
    touchLEDSensor.setBlink(calLedColour(target), 0.15, 0.15);
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
    wait(WAIT_TIME_MS, msec);
  }
  touchLEDSensor.setBrightness(0);
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
    gState = STATE_COLOUR_CHECK;
  }
}

void handleColourCheck(void) {
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
    gNextDir = static_cast<Heading>((gHeading - 1) % 4);
    break;
  case RIGHT:
    gNextDir = static_cast<Heading>((gHeading + 1) % 4);
    break;
  case IDLE:
    return;
  default:
    gNextDir = gHeading;
  }
  gState = STATE_MOVE;
}

void handleMove(void) {
  touchLEDSensor.on(green);

  if (gNextDir != gHeading) {
    Drivetrain.turnToHeading((int)gNextDir * QUARTER_TURN, degrees);
    gHeading = gNextDir;
  } else {
    Drivetrain.driveFor(forward, CELL_SIZE_MM, mm);
  }
  // EXTENSION POINT 3: collision detection goes here
  gState = STATE_COLOUR_CHECK;
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
  Brain.Screen.print("place on center of %i, %i faceing %c", info.x, info.y,
                     info.heading);
  while (!touchLEDSensor.pressing())
    wait(WAIT_TIME, msec);

  // Enters loop agian at correct XY
  gState = STATE_COLOUR_CHECK;
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
  case STATE_COLOUR_CHECK:
    handleColourCheck();
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
