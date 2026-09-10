#include "API/API.h"
#include "navigation.h"
#include <stdio.h>

// You do not need to edit this file.
// This program just runs your solver and passes the choices
// to the simulator.
int main(int argc, char *argv[]) {
  Action decsion = decide(API_wallFront());
  switch (decsion) {
  case LEFT:
    API_turnLeft();
    API_moveForward();
    break;
  case RIGHT:
    API_turnRight();
    API_moveForward();
    break;
  default:
    API_moveForward();
    break;
  }
}
