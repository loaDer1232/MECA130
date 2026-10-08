/*----------------------------------------------------------------------------*/
/* */
/* Module: main.cpp */
/* Author: kinganseam erasmuchri */
/* Created: 03/09/2026, 11:00:00 */
/* Description: IQ2 project */
/* Final Project MECA130*/
/*----------------------------------------------------------------------------*/
#include "ColorCalibration.h"
#include "navigation.h"
#include "vex.h"
#include <cmath>

#define LOOP_DELAY 200

#define WALL_SET_MM 300
#define WALL_CLEAR_MM 320
#define WAIT_TIME 200
#define SCREEN_RESET_X 1
#define SCREEN_RESET_Y 1

#define BUFFER_SIZE 3

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
// Smart Vex Device Setup
vex::brain Brain;
touchled touchLEDSensor = touchled(PORT10);
optical opticalSensor = optical(PORT1);
motor leftMotor =
    motor(PORT6, false); // standard direction check port***   task 1.4
motor rightMotor = motor(PORT12, true);   // reversed (mirrored mounting)
inertial brainInertial = inertial(right); // task 1.4
smartdrive Drivetrain = smartdrive(       // task 1.4
    leftMotor, rightMotor, brainInertial, WHEEL_CIRCUMFERENCE, TRACK_WIDTH_MM,
    WHEEL_BASE_MM, mm, GEAR_RATIO); // task 1.4

distance distanceSensor = distance(PORT5);
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

static const CalColor gCalOrder[] = {COLOR_BLACK, COLOR_RED,    COLOR_GREEN,
                                     COLOR_BLUE,  COLOR_YELLOW, COLOR_WHITE};
static const int CAL_STEPS = (int)(sizeof(gCalOrder) / sizeof(gCalOrder[0]));

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
    gNextDir = static_cast<Heading>((getHeading() - 1) % 4);
    break;
  case RIGHT:
    gNextDir = static_cast<Heading>((getHeading() + 1) % 4);
    break;
  case IDLE:
    return;
  default:
    gNextDir = 0;
  }
  gState = STATE_MOVE;
}

void handleMove(void) {
  touchLEDSensor.on(green);

  if (gNextDir != getHeading()) {
    Drivetrain.turnToRotation((int)gNextDir * QUARTER_TURN, degrees);
    gNextDir = getHeading();
  } else {
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
  Brain.Screen.print("place on center of %i, %i faceing %c", info.cell.x,
                     info.cell.y, info.heading);
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
