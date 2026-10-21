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

#define LOOP_DELAY 200

#define IDEAL_DIST 94

#define WALL_SET_MM 300
#define WALL_CLEAR_MM 390
#define SCREEN_RESET_X 1
#define SCREEN_RESET_Y 1

#define BUFFER_SIZE 5

#define QUARTER_TURN 90

#define GEAR_RATIO 1
#define WHEEL_DIAMETER_MM 63.6
#define WHEEL_CIRCUMFERENCE (3.14159 * WHEEL_DIAMETER_MM)
#define TRACK_WIDTH_MM 18.63
#define WHEEL_BASE_MM 121.7
#define CELL_SIZE_MM 400.0 // adjust to r/w measurement

#define INERTIAL_TOLERANCE 2

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

using namespace vex;

// Smart Vex Device Setup
vex::brain Brain;
touchled touchLEDSensor = touchled(PORT10);
optical opticalSensor = optical(PORT1);
motor leftMotor = motor(PORT6, false);  // standard direction check port***
motor rightMotor = motor(PORT12, true); // reversed (mirrored mounting)
inertial brainInertial = inertial(right);
smartdrive Drivetrain =
    smartdrive(leftMotor, rightMotor, brainInertial, WHEEL_CIRCUMFERENCE,
               TRACK_WIDTH_MM, WHEEL_BASE_MM, mm, GEAR_RATIO);

distance distanceSensor = distance(PORT5);
bumper bumpSensor = bumper(PORT11);

double buffer[BUFFER_SIZE];
int writeIndex = 0;
int count = 0;

double offsetCalc(double h1, double h2) {
  double numer = (h2 * cos(20) - h1);
  double denom = (h2 * sin(20));
  double offset = atan2(numer, denom);
  return offset;
}

void center() {
  Drivetrain.turnFor(20, deg);
  double h1 = distanceSensor.objectDistance(mm);
  Drivetrain.turnFor(-40, deg);
  double h2 = distanceSensor.objectDistance(mm);
  Drivetrain.turnFor(20, deg);
  double x = offsetCalc(h1, h2) * (PI / 180);
  Drivetrain.turnFor(x, deg);
}

void bufferWrite(double value) {
  // Ignore invalid/out-of-range sensor readings (0 or max range)
  if (value <= 0)
    return;

  buffer[writeIndex] = value;
  writeIndex = (writeIndex + 1) % BUFFER_SIZE;
  if (count < BUFFER_SIZE)
    count++;
}

double bufferAverage(void) {
  if (count == 0)
    return 0.0; // Prevent division by zero

  double total = 0.0;
  for (int i = 0; i < count; i++) {
    total += buffer[i];
  }
  return total / count;
}

bool updateWallDetection(double avg) {
  static bool wallDetected = false;

  // If avg is 0, the sensor isn't seeing anything valid
  if (avg == 0)
    return false;

  if (wallDetected) {
    if (avg > WALL_CLEAR_MM)
      wallDetected = false;
  } else if (avg < WALL_SET_MM) {
    wallDetected = true;
  }
  return wallDetected;
}

static bool flashed[MAZE_WIDTH][MAZE_HEIGHT] = {{false}};
static void flashTile(TileColor c) {
  color led;
  switch (c) {
  case RED:
    led = red;
    break;
  case GREEN:
    led = green;
    break;
  case BLUE:
    led = blue;
    break;
  case YELLOW:
    led = yellow;
    break;
  default:
    return; // black / white: nothing to report
  }
  touchLEDSensor.setBlink(led, 0.25, 0.25);
  wait(800, msec);
  touchLEDSensor.setBrightness(0);
}

RobotState gState = STATE_COLOR_CAL;
TileColor gtileColor;

static Heading lastHeading = getHeading();
static bool gWallAhead = false;
static Heading targetHeading = getHeading();

void screen(float avgDist) {
  Info info = getInfo();

  const char *str = "UN";

  switch (info.cell.type) {
  case TILE_NORMAL:
    str = "Normal";
    break;
  case TILE_START:
    str = "Start";
    break;
  case TILE_HAZARD:
    str = "Hazard";
    break;
  case TILE_ASSEMBLY:
    str = "Asm";
    break;
  case TILE_SURVIVOR:
    str = "Dude";
    break;
  case TILE_CACHE:
    str = "Cache";
    break;
  case TILE_UNKNOWN:
  default:
    str = "UN";
    break;
  }
  touchLEDSensor.setBrightness(0);
  Brain.Screen.clearScreen();
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Dist: %.1f", avgDist);
  Brain.Screen.setCursor(2, 1);
  Brain.Screen.print("wall: %d", gWallAhead);
  Brain.Screen.setCursor(3, 1);
  Brain.Screen.print("x: %d, y: %d", info.cell.x, info.cell.y);
  Brain.Screen.setCursor(4, 1);
  Brain.Screen.print("Type: %s", str);
}

// --- Handlers ---

void handleInit(void) {
  touchLEDSensor.on(white);
  Brain.Screen.clearScreen();
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Calibrating...");

  brainInertial.calibrate();
  while (brainInertial.isCalibrating()) {
    wait(WAIT_TIME, msec);
  }
  gState = STATE_IDLE;
}

void handleColorCal() {
  touchLEDSensor.on(white);
  Brain.Screen.clearScreen();
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Color Calibration");
  Brain.Screen.setCursor(2, 1);
  Brain.Screen.print("Press LED to start,");
  Brain.Screen.setCursor(3, 1);
  Brain.Screen.print("Press Bumper to");
  Brain.Screen.setCursor(4, 1);
  Brain.Screen.print("use preset defaults");
  if (touchLEDSensor.pressing()) {
    colorCal();
  } else if (bumpSensor.pressing()) {
    Brain.Screen.clearScreen();
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("Using Defaults");
    wait(WAIT_TIME, msec);
    loadDefaultCol(); // fills gCal from defColor array
    gState = STATE_INIT;
  }

  void handleIdle(void) {
    touchLEDSensor.on(white);
    Brain.Screen.clearScreen();
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("Press LED to start");
    while (!touchLEDSensor.pressing()) {
      wait(WAIT_TIME, msec);
    }
    gState = STATE_COLOR_CHECK;
  }

  void handlecolorCheck(void) {
    touchLEDSensor.on(purple);
    gtileColor =
        classifyTileColor(opticalSensor.hue(), opticalSensor.brightness());

    Info info = getInfo(); // cell the robot is standing on
    if (!flashed[info.cell.x][info.cell.y]) {
      flashed[info.cell.x][info.cell.y] = true;
      flashTile(gtileColor);
    }

    // EXTENSION POINT 2: objective handler states branch from here
    gState = STATE_WALL_CHECK;
  }

  void handleWallCheck(void) {
    touchLEDSensor.on(blue_green);
    count = 0;
    writeIndex = 0;

    // The robot is stationary here, so averaging is valid
    for (int i = 0; i < BUFFER_SIZE; i++) {
      bufferWrite(distanceSensor.objectDistance(mm));
      wait(WAIT_TIME / BUFFER_SIZE, msec);
    }

    double avgDist = bufferAverage();
    gWallAhead = updateWallDetection(avgDist);
    screen(avgDist);
    gState = STATE_DECIDE;
  }

  static Action gAction = IDLE;

  void handleDecide(void) {
    touchLEDSensor.on(yellow);
    gAction = decide(gWallAhead, gtileColor);
    targetHeading = getHeading();
    gState = (gAction == IDLE) ? STATE_IDLE : STATE_MOVE;
  }

  void handleMove(void) {
    touchLEDSensor.on(green);
    if (lastHeading != targetHeading) {
      Drivetrain.turnToRotation((int)targetHeading * QUARTER_TURN, degrees);
      lastHeading = targetHeading;
    }

    if (gAction == FORWARD || gAction == REVERSE)
      Drivetrain.driveFor(forward, CELL_SIZE_MM, mm);

    if (distanceSensor.objectDistance(mm) > IDEAL_DIST &&
        distanceSensor.objectDistance(mm) < 200)
      Drivetrain.driveFor(forward,
                          (distanceSensor.objectDistance(mm) - IDEAL_DIST), mm);

    if (distanceSensor.objectDistance(mm) < IDEAL_DIST)
      Drivetrain.driveFor(reverse,
                          (IDEAL_DIST - distanceSensor.objectDistance(mm)), mm);
    center();
    gState = STATE_COLOR_CHECK;
  }

  void handleError(void) {
    Drivetrain.stop();

    touchLEDSensor.on(red);
    // I dont like that I am calling for the debug object I made
    Info info = getInfo();
    wait(WAIT_TIME, msec);
    Brain.Screen.setCursor(2, 1);
    Brain.Screen.print("x: %d, y: %d", info.cell.x, info.cell.y);
    Brain.Screen.setCursor(3, 1);
    Brain.Screen.print("place on center of %i, %i faceing %c", info.cell.x,
                       info.cell.y, info.heading);
    while (!touchLEDSensor.pressing())
      brainInertial.calibrate();
    do {
      wait(WAIT_TIME, msec);
    } while (brainInertial.isCalibrating());

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
    Drivetrain.setTurnVelocity(TURN_SPEED, percent);
    Drivetrain.setDriveVelocity(DRIVE_SPEED, percent);
    while (1) {
      runStateMachine();
      wait(LOOP_DELAY, msec);
    }
  }
