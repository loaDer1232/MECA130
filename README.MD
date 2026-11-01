# MECA130 Mouse Robot
[[_TOC_]]

## Project Description

### Challenge Brief
An earthquake has struck. A building has been damaged and there are people trapped inside who cannot move. Others have made their own way to an assembly point and are waiting.

The building is too unsafe to send anybody in, so an autonomous robot goes instead. Once it is inside, nobody can help it. Every decision it makes is one you wrote beforehand.

### What the Robot has to do
Mission objectives, in priority order (from Lecture 11A). Not every objective has to be attempted.
1. Find every survivor (blue tiles).
2. Reach the assembly point (green tile). It only counts once all survivors are found, so the robot keeps searching first.
3. Evacuate to the deployment tile (white), the only known good exit.
4. Identify collapse zones (red tiles) and record where they were.
5. Collect supply caches (yellow tiles). A bonus that must not delay the primary mission.
6. Recover from collisions.

### Scoring
Each tile counts once. 1000 points in total.

| action                          | points |
|---------------------------------|--------|
| Find survivor (3)               | +100   |
| Reach assembly point            | +300   |
| Deploy and exit                 | +200   |
| Find supply cache (4)           | +50    |
| Enter, report and reverse out of a collapse zone | 0 |
| Drive through a collapse zone without noticing   | -25 |
| Return to a collapse zone       | -25    |


A tile only counts if the robot **pauses and flashes** that colour on the touchLED over it. Driving over a survivor in silence scores nothing. Every reset costs marks.

### Maze and Run Rules
- L-shaped maze, 60 cells: ten by five, with a five by two extension. 400 mm cells, opaque walls 18 to 20 mm thick, closed outer boundary. Use a 10 x 10 grid for the map (some of it is never visited).
- The robot is deployed on the white tile in the bottom-left corner and must exit there.
- Sample mazes contain 3 survivors, 4 caches, 3 collapse zones and 1 assembly point.
- Ten minute time limit, one scored run, identical robots, fixed VEX ports.
- Two maze types, chosen before the run: the **tethered** maze (one wall segment joins the island to the outer boundary, solvable by wall following, mark capped at 74%) or the **island** maze (needs a map, up to 100%). A robot that only reacts is capped at 74%, a robot that remembers where it has been can reach 100%.
- Assessment: the robot is 60% (pair mark, based on performance) and the viva is 40% (individual). The demonstration is on 2 Nov.

### Tile Colors
| color  | meaning        | `Tile` type     |
|--------|----------------|-----------------|
| black  | normal floor   | `TILE_NORMAL`   |
| white  | start / exit   | `TILE_START`    |
| red    | hazard         | `TILE_HAZARD`   |
| green  | assembly point | `TILE_ASSEMBLY` |
| blue   | survivor       | `TILE_SURVIVOR` |
| yellow | supply cache   | `TILE_CACHE`    |

Counts and maze size used by the objective logic are set in `GlobalDefs.h` (currently a 16x16 map with 3 survivors, 5 caches, 0 hazards and 1 assembly tile). Drive and turn speeds (`DRIVE_SPEED` 72%, `TURN_SPEED` 92%) are defined there too. For the real maze these should be 3 survivors, 4 caches and 3 hazards; see Known Limitations.

## Hardware

| device               | port / name          | notes                                  |
|----------------------|----------------------|----------------------------------------|
| Optical sensor       | `PORT1`              | tile hue and brightness, light on      |
| Distance sensor      | `PORT5`              | wall detection ahead                   |
| Left motor           | `PORT6`              | normal direction                       |
| Right motor          | `PORT12`             | reversed (mirrored mounting)           |
| Touch LED            | `PORT10`             | user input and status light            |
| Bumper               | `PORT11`             | skip input during calibration          |
| Inertial sensor      | `right` (brain)      | heading for `smartdrive` turns         |

Drivetrain: `smartdrive` with a 63.6 mm wheel and a 400 mm cell size (`CELL_SIZE_MM` in `main.cpp`, adjust to the real maze).

## Code Layout
| file                  | purpose                                                         |
|-----------------------|-----------------------------------------------------------------|
| `main.cpp`            | Robot state machine, hardware setup, drive and wall handling    |
| `ColorCalibration.*`  | Hue and brightness calibration and tile classification          |
| `navigation.*`        | Maze map, exploration, path planning, objective logic           |
| `GlobalDefs.*`        | Shared constants and the `TileColor` enum                       |
| `micromouse.cpp`      | Simulator entry point (only compiled with `Micromouse` defined) |
| `dataStructs/`        | `Stack` and `Queue` used by navigation                          |

## Design

### States
The program starts in `color calibration`. Every state is a handler function in `main.cpp`, called once per loop by `runStateMachine()`.

The brief requires a state machine with at least these states, which map onto ours as follows:

| required state | our state(s)                                |
|----------------|---------------------------------------------|
| initialise     | `color calibration`, `init`                 |
| idle           | `idle`                                      |
| scan           | `wall check`                                |
| read tile      | `color check` (flash is done in `screen()`) |
| decide         | `decide`                                    |
| drive          | `move`                                      |
| halt           | `idle`                                      |

The touchLED must also show what the robot is doing: each state sets its own LED colour.

#### State Transition Table
| current state     | event                         | next state        |
|-------------------|-------------------------------|-------------------|
| color calibration | calibration done or skipped   | init              |
| init              | inertial sensor calibrated    | idle              |
| idle              | touch LED pressed             | color check       |
| color check       | tile type classified          | wall check        |
| wall check        | wall-ahead result stored      | decide            |
| decide            | action chosen                 | move              |
| decide            | `IDLE` (run finished)         | idle              |
| move              | turn or drive complete        | color check       |
| error             | placed on correct cell, press | color check       |

Nothing currently transitions into `error`, and the recovery idea has been removed from the code. When `decide()` returns `IDLE` the robot goes back to `idle` and waits for a press.

#### State Diagram
![state machine diagram](./media/state-diagram.png)

### Wall Detection
`wall check` stops the robot and takes `BUFFER_SIZE` (5) distance readings, discarding invalid readings of 0 or less. The average goes through a hysteresis filter: a wall is set below `WALL_SET_MM` (300 mm) and only cleared above `WALL_CLEAR_MM` (390 mm), so readings near the threshold do not flicker. The result is shown on the brain screen along with the position and tile type, and the touch LED blinks the color of any special tile.

### Color Calibration
Calibration runs at startup so the robot does not depend on hard-coded hue ranges that change with lighting.

**Procedure**
1. Tiles are calibrated in this order: black, red, green, blue, yellow, white.
2. Place the optical sensor over the prompted tile. The brain screen shows live hue, brightness and the current prediction.
3. Press the **touch LED** to save that tile, or the **bumper** to skip it and leave it uncalibrated.
4. Pressing the **bumper** on the very first (black) step skips the whole calibration.
5. After the last tile, place the robot on the start tile facing the start direction and press the touch LED to begin.

**How it works**
- **Hue is circular.** `wrapHue`, `hueDistance` and `circularMean` handle the 0/360 seam, so red samples such as 356 and 4 average to about 0, not 180.
- **Colored tiles** take `NUM_SAMPLES` (10) readings. The band center is the circular mean. The tolerance is the widest spread seen plus 10 degrees, clamped to 10-45 degrees.
- **Black** is decided by brightness only. The threshold is the average black brightness plus `BLACK_MARGIN`, clamped to `BLACK_THRESH_MIN`-`BLACK_THRESH_MAX`. If black calibration is skipped, `BRIGHT_LOW` is the fallback.
- **Classification** (`classifyTileColor`): below the black threshold is black. Otherwise the nearest calibrated hue center wins if it is inside its tolerance band. Anything outside every band falls back to white.

### Navigation
Navigation keeps a 16x16 map of `Cell`s and decides one action per call to `decide(wallFront, color)`. Each cell stores its tile type and four walls, and each wall is `TRUE`, `FALSE` or `UNKNOWN`. The map border is pre-set to walls in `init()`.

1. **Map the cell.** On the first visit the robot records the wall ahead, then turns only toward sides whose wall is still `UNKNOWN` (right, then left, then behind with a `FLIP`), recording the wall ahead each time. Walls are written to both neighbouring cells. Once every side is known, the tile type is recorded and objectives are counted.
2. **Handle hazards.** A red tile is marked `TILE_HAZARD`, counted, and the robot reverses out immediately (`REVERSE`: turn around and drive one cell). The stored position steps back one cell to match. Hazard cells are never used for paths or exploration targets.
3. **Choose a target.**
   - Explore: unvisited reachable neighbours are pushed on a stack and the next one is popped as the target (depth-first exploration).
   - Once the assembly tile is found and all survivors and caches are found, go to the assembly tile, then return to the start cell `(0, 0)`.
4. **Plan the path.** Breadth-first search over mapped, non-hazard cells builds a queue of `LEFT`, `RIGHT` and `FORWARD` actions to the target.
5. **Execute.** One queued action is returned per call, and the stored position and heading are updated to match.

`decide()` returns `IDLE` only when mapping of the current cell is finished, the run is complete, or nothing is left to explore.

### Driving
`handleDecide` stores the action returned by `decide()` and the new heading. `handleMove` first turns if the heading changed (absolute rotation of `heading * 90` degrees on the inertial sensor), then drives one cell forward only if the action was `FORWARD` or `REVERSE`.

| action    | turns        | moves one cell | `updateXY()` |
|-----------|--------------|----------------|--------------|
| `FORWARD` | no           | yes            | yes          |
| `LEFT`    | 90° left     | no             | no           |
| `RIGHT`   | 90° right    | no             | no           |
| `FLIP`    | 180°         | no             | no           |
| `REVERSE` | 180°         | yes            | yes          |
| `IDLE`    | no           | no             | no           |

**Position rule:** an action that moves the robot one cell must call `updateXY()`, and one that does not must leave it alone. This must hold in the robot (`handleMove`) and in the simulator (`micromouse.cpp`) alike.

## Simulation
Using a simulator is optional and nothing is assessed on it. The navigation logic can be run in the [Micromouse simulator](https://github.com/mackorone/mms) without the robot. `micromouse.cpp` is wrapped in `#ifdef Micromouse`, so the macro must be defined.
![MicroMouse](./media/maze_sim.gif)
The simulator and the program talk over stdin and stdout, so anything printed to stdout is read as a command. Debug output must go through `debug_log` (stderr), never `printf`.

The course also provides its own kit (`robotapi.h` with `robotapi_sim.cpp` and `robotapi_vex.cpp`, built with `build.sh`). This project uses the mms API directly instead, and `navigation.cpp` has no hardware calls so it builds either way.

Build command:
```
gcc -D Micromouse -o  a.exe ./API/API.cpp ./dataStructs/Stack.cpp ./dataStructs/Queue.cpp micromouse.cpp navigation.cpp
```
The simulator draws each action with the API calls in `micromouse.cpp`, following the action table above.

In the simulator, tile colors are placed randomly by `mazeInit()` using the counts in `GlobalDefs.h`, and a debug string with position, target and objectives found is logged each step.

## Known Limitations / TODO
- `error` is unreachable and there is no recovery or collision detection (extension point in `handleMove`).
- There is no `halt` state and no bumper collision recovery (the lowest priority objective).
