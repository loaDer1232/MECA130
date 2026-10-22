#include "music.h"

void playSong(Song song) {
  for (int i = 0; i < song.duration; i++) {
    Brain.playNote(4, song.notes[i].pitch,
                   (1 / (song.notes[i].length)) * song.tempo);
  }
