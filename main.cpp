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

#define PI = 3.14159265358979323846

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

TileColor classifyTileColor(double hue, double bright) {
  // if (bright < BRIGHT_LOW) {
  //   return BLACK;
  // }
  if (hue > redHue.min || hue < redHue.max) {
    return RED;
  }
  if (hue > greenHue.min && hue < greenHue.max) {
    return GREEN;
  }
  if (hue > blueHue.min && hue < blueHue.max) {
    return BLUE;
  }
  if (hue > yellowHue.min && hue < yellowHue.max) {
    return YELLOW;
  }
  return WHITE;
}

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

HueValues colorAvrager() {
  double sumSin = 0.0, sumCos = 0.0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    double rad = opticalSensor.hue() * PI / 180.0;
    sumSin += sin(rad);
    sumCos += cos(rad);
    wait(20, msec);
  }
  double mean = atan2(sumSin, sumCos) * 180.0 / PI;
  if (mean < 0)
    mean += 360.0;

  HueValues hue = {fmod(mean + 10, 360.0), fmod(mean - 10 + 360.0, 360.0)};
  return hue;
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

void handleColorCal() {
  touchLEDSensor.on(black); // corridor
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("place on black square");

  // Normal floor
  while (!touchLEDSensor.pressing())
    wait(WAIT_TIME, msec);
  // TODO black calbration
  touchLEDSensor.on(red);
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("place on red square");
  wait(WAIT_TIME, msec);

  // Hazard
  while (!touchLEDSensor.pressing())
    wait(WAIT_TIME, msec);
  redHue = colorAvrager();
  touchLEDSensor.on(green);
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("place on green square");
  wait(WAIT_TIME, msec);

  // Assembly point
  while (!touchLEDSensor.pressing())
    wait(WAIT_TIME, msec);
  greenHue = colorAvrager();
  touchLEDSensor.on(blue);
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("place on blue square");
  wait(WAIT_TIME, msec);

  // peeps
  while (!touchLEDSensor.pressing())
    wait(WAIT_TIME, msec);
  blueHue = colorAvrager();
  touchLEDSensor.on(yellow);
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("place on yellow square");
  wait(WAIT_TIME, msec);

  // Cache
  while (!touchLEDSensor.pressing())
    wait(WAIT_TIME, msec);
  yellowHue = colorAvrager();
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("place on white square");
  touchLEDSensor.on(white);
  wait(WAIT_TIME, msec);

  // Start/finish point
  while (!touchLEDSensor.pressing())
    wait(WAIT_TIME, msec);

  gState = STATE_COLOUR_CHECK;
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
  // gState = STATE_WALL_CHECK;
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
  // touchLEDSensor.on(yellow);
  // Action decsion = decide(gWallAhead, gtileColor);
  // switch (decsion) {
  // case LEFT:
  //  gNextDir = (Direction)((gHeading - 1) % 4);
  // break;
  // case RIGHT:
  //  gNextDir = (Direction)((gHeading + 1) % 4);
  //  break;
  // default:
  // gNextDir = gHeading;
  //}
  // gState = STATE_MOVE;
}

void handleMove(void) {
  touchLEDSensor.on(green);

  if (gNextDir != gHeading) {
    Drivetrain.turnToHeading((int)gNextDir * QUARTER_TURN, degrees);
    gHeading = gNextDir;
  }

  Drivetrain.driveFor(forward, CELL_SIZE_MM, mm);

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
  while (1) {
    runStateMachine();
    wait(LOOP_DELAY, msec);
  }
}
