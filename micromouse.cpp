#include "API/API.h"
#include "navigation.h"

int main() {
  while (1) {
    Action decsion = decide(API_wallFront());
    switch (decsion) {
    case FORWARD:
      API_moveForward();
      debug_log(const_cast<char *>("Moving forward"));
      break;
    case LEFT:
      API_turnLeft();
      debug_log(const_cast<char *>("Turning left"));
      break;
    case RIGHT:
      API_turnRight();
      debug_log(const_cast<char *>("Turing right"));
      break;
    default:
      debug_log(const_cast<char *>("undefined state"));
      break;
    }
  }
}
