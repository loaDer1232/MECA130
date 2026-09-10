/*----------------------------------------------------------------------------*/
/* */
/* Module: main.cpp */
/* Author: kinganseam erasmuchri */
/* Created: 03/09/2026, 11:00:00 */
/* Description: IQ2 project */
/* Lab 6*/
/*----------------------------------------------------------------------------*/
#include "vex.h"

#define LED_PRESSED 1
#define LED_RELEASED 0

#define LOOP_DELAY 200

#define WALL_SET_MM 300
#define WALL_CLEAR_MM 320
#define WAIT_TIME 200
#define SCREEN_RESET_X 1
#define SCREEN_RESET_Y 1
#define NUM_SAMPLES 10

#define BUFFER_SIZE 1
#define BRIGHT_LOW 10
#define RED_HUE_MIN 340
#define RED_HUE_MAX 20
#define GREEN_HUE_MIN 50
#define GREEN_HUE_MAX 60
#define BLUE_HUE_MIN 60
#define BLUE_HUE_MAX 90
#define YELLOW_HUE_MIN 40
#define YELLOW_HUE_MAX 75
#define QUARTER_TURN 90 // task 4
#define FULL_CIRCLE 360

#define WHEEL_DIAMETER_MM 63.6                            // task 1.1.1
#define WHEEL_CIRCUMFERENCE (3.14159 * WHEEL_DIAMETER_MM) // task 1.2
#define TRACK_WIDTH_MM 18.63                              // task 1.3
#define WHEEL_BASE_MM 121.7                               // task 1.3
#define GEAR_RATIO 1.0                                    // always direct drive
#define CELL_SIZE_MM 300.0 // adjust to r/w measurement

#define INERTIAL_TOLERANCE 2
#define HEADING_NORTH 0
#define HEADING_EAST 90
#define HEADING_SOUTH 180
#define HEADING_WEST 270

typedef enum {
  TILE_NORMAL,
  TILE_FINISH,
  TILE_CHECKPOINT,
  TILE_WAYPOINT,
  TILE_HAZARD,
  TILE_START200
} TileType;

typedef enum {
  STATE_INIT,
  STATE_IDLE,
  STATE_COLOUR_CHECK,
  STATE_WALL_CHECK,
  STATE_DECIDE,
  STATE_MOVE,
  STATE_ERROR
} RobotState;

TileType classifyTile(double hue, double bright) {
  if (bright < BRIGHT_LOW) {
    return TILE_NORMAL;
  }
  if (hue > RED_HUE_MIN || hue < RED_HUE_MAX) {
    return TILE_HAZARD;
  }
  if (hue > GREEN_HUE_MIN && hue < GREEN_HUE_MAX) {
    return TILE_FINISH;
  }
  if (hue > BLUE_HUE_MIN && hue < BLUE_HUE_MAX) {
    return TILE_CHECKPOINT;
  }
  if (hue > YELLOW_HUE_MIN && hue < YELLOW_HUE_MAX) {
    return TILE_WAYPOINT;
  }
  return TILE_NORMAL;
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
touchled TouchLED1 = touchled(PORT10);
optical opticalSensor = optical(PORT1);

bumper Bumper1 = bumper(PORT11);

double buffer[BUFFER_SIZE];
int writeIndex = 0;
int count = 0;

int led_released = LED_RELEASED;

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

bool wallDetected = false;
void updateWallDetection(double avg) {
  if (wallDetected) {
    if (avg > WALL_CLEAR_MM)
      wallDetected = false;
  } else {
    if (avg < WALL_SET_MM)
      wallDetected = true;
  }
}
static RobotState gState = STATE_INIT;
static Direction gHeading = NORTH;
static Direction gNextDir = NORTH;
static bool gWallAhead = false;

// --- Handlers ---

void handleInit(void) {
  touchLEDSensor.on(purple);
  Brain.Screen.clearScreen();
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Calibrating...");

  brainInertial.calibrate();
  do {
    wait(WAIT_TIME_MS, msec);
  } while (brainInertial.isCalibrating());

  gHeading = NORTH;
  gState = STATE_IDLE;
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
  TileType tile = classifyTile(opticalSensor.hue(), opticalSensor.brightness());

  Brain.Screen.clearScreen();
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Tile: %d", tile);

  // EXTENSION POINT 2: objective handler states branch from here
  gState = STATE_WALL_CHECK;
}

void handleWallCheck(void) {
  touchLEDSensor.on(blue_green);

  // The robot is stationary here, so averaging is valid
  for (int i = 0; i < BUFFER_SIZE; i++) {
    bufferWrite(distanceSensor.objectDistance(mm));
    wait(READING_DELAY, msec);
  }
  updateWallDetection(bufferAverage());
  gWallAhead = wallDetected;

  gState = STATE_DECIDE;
}

void handleDecide(void) {
  touchLEDSensor.on(yellow);

  // EXTENSION POINT 1: your navigation algorithm replaces this rule
  if (gWallAhead) {
    gNextDir = (Direction)((gHeading + 1) % 4); // turn right
  } else {
    gNextDir = gHeading; // carry straight on
  }

  gState = STATE_MOVE;
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

void handleError(void) {
  touchLEDSensor.on(red);

  if (gNextDir != gHeading) {
    Drivetrain.turnToHeading((int)gNextDir * QUARTER_TURN, degrees);
    gHeading = gNextDir;
  }

  Drivetrain.driveFor(forward, CELL_SIZE_MM, mm);

  // EXTENSION POINT 3: collision detection goes here
  gState = STATE_COLOUR_CHECK;
}

// --- Dispatcher ---

void runStateMachine(void) {
  switch (gState) {
  case STATE_INIT:
    handleInit();
    break;
  case STATE_IDLE:
    handleIdle();LOOP_DELAY
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
