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
