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
typedef enum Length { EIGHTTH, QUATER, HALF, WHOLE } Length;

typedef struct Note {
  Pitch pitch;
  Length Length;
} Note;

typedef struct Song {
  int Tempo = 120;
  int duration;
  Note[256] notes;
} Song;

#endif
