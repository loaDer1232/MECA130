# meca130 mouse robot
[[_TOC_]]
##Project Description
###Challange Brief
An earthquake has struck
A building has been damaged and there are
people trapped inside who cannot move. Others
have made their own way to an assembly point
and are waiting.
The building is too unsafe to send anybody in,
so an autonomous robot goes instead. Once it
is inside, nobody can help it. Every decision it
makes is one you wrote beforehand.

###What the Robot has to do
1. Search the building and find every survivor(blue tiles).
2. Avoid the sections that are still collapsing(red tiles), and record where they were.
4. Gather everybody at the assembly point, which is the green tile.
5. Evacuate: find its own way back out through the entry point it came in by.
6. Collect supply caches(yellow tiles) along the way if the route allows.

###Scoring
| action                 | points |
|------------------------|--------|
| Find survivor          | +100   |
| Find cache             | +50    |
| deploy and exit        | +200   |
| react to collapse      | +0     |
| drive through collapse | -25    |
| return to collapse     | -25    |

## Design
### States
#### State Transistion Table
| current state     | event                  | next state  |
|-------------------|------------------------|-------------|
| color calibration | Complete               | init        |
| init              | Complete               | idle        |
| idle              | LED Pressed            | color check |
| color check       | get tile type          | wall check  |
| wall check        | return wall front      | decide      |
| decide            | decision               | move        |
| move              | move Complete          | color check |
| recovry           | recovers               | color check |
| recovry           | fails                  | error       |
| error             | return to correct cell | color check |

####State Diagram
![state machine diagram](./Assets/statemachine.png)
### Navigation


## requirements for simulation
https://github.com/mackorone/mms to run micromouse
use build command 
```
gcc -o a.exe ./API/API.cpp micromouse.cpp navigation.cpp
```
