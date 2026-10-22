#ifndef MUSIC_H
#define MUSIC_H

typedef enum Pitch {
  C,
  D,
  E,
  F,
  G,
  A,
  B,
} Pitch;
typedef enum Length { EIGHTTH = 8, QUATER = 4, HALF = 2, WHOLE = 1 } Length;

typedef struct Note {
  Pitch pitch;
  Length Length;
} Note;

typedef struct Song {
  int tempo = 120;
  int duration;
  Note notes[256];
} Song;

#endif
