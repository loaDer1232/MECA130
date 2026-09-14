#include "API/API.h"
#include "navigation.h"

int main() {
  while (1) {
    Info info = getInfo();
    Action decsion = decide(API_wallFront());
    switch (decsion) {
    case FORWARD:
      debug_log(&info.color);
      API_setColor(info.x, info.y, info.color);
      API_moveForward();
      debug_log(const_cast<char *>("Moving forward"));
      break;
    case LEFT:
      API_setWall(info.x, info.y, info.heading);
      debug_log(const_cast<char *>(&info.heading));
      API_turnLeft();
      debug_log(const_cast<char *>("Turning left"));
      break;
    case RIGHT:
      API_setWall(info.x, info.y, info.heading);
      debug_log(const_cast<char *>(&info.heading));
      API_turnRight();
      debug_log(const_cast<char *>("Turing right"));
      break;
    default:
      debug_log(const_cast<char *>("undefined state"));
      break;
    }
  }
}
