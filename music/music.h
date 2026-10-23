#ifndef MUSIC_H
#define MUSIC_H

// Octave range accepted by Brain.playNote.
#define MIN_OCTAVE 0
#define MAX_OCTAVE 8

typedef enum Pitch { C = 0, D = 1, E = 2, F = 3, G = 4, A = 5, B = 6 } Pitch;

typedef enum Length { EIGHTH = 8, QUARTER = 4, HALF = 2, WHOLE = 1 } Length;

typedef struct Note {
  int octave;
  Pitch pitch;
  Length length;
} Note;

typedef struct Song {
  int tempo;    // beats per minute
  int duration; // number of notes used
  Note notes[256];
} Song;

// Robot-side functions (defined in music.cpp)
void playSong(const Song &song);
Song readSong(const char *file);

#endif
