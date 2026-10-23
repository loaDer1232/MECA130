#include "music.h"
#include "vex.h"
using namespace vex;

// File format: 4-byte LE tempo, 4-byte LE note count, then 3 bytes per note
// (octave, pitch semitone, length).
static const int HEADER_SIZE = 8;
static const int NOTE_SIZE = 3;
static const int MAX_NOTES = 256;
static const int MAX_FILE_SIZE = HEADER_SIZE + MAX_NOTES * NOTE_SIZE;

static int readInt32(const uint8_t *p) {
  return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static bool validOctave(int o) { return o >= MIN_OCTAVE && o <= MAX_OCTAVE; }

static bool validPitch(int p) {
  return p == C || p == D || p == E || p == F || p == G || p == A || p == B;
}

static bool validLength(int l) {
  return l == WHOLE || l == HALF || l == QUARTER || l == EIGHTH;
}

void playSong(const Song &song) {
  // A whole note is 4 beats; one beat lasts 60000 / tempo ms.
  const int wholeNoteMs = 4 * 60000 / song.tempo;

  for (int i = 0; i < song.duration; i++) {
    int ms = wholeNoteMs / song.notes[i].length;
    Brain.playNote(song.notes[i].octave, song.notes[i].pitch, ms);
    wait(ms, msec); // playNote returns immediately, so wait for it to finish
  }
}

Song readSong(const char *file) {
  Song song = {};
  song.tempo = 120;
  song.duration = 0;

  if (!Brain.SDcard.exists(file))
    return song;

  int fileSize = Brain.SDcard.size(file);
  if (fileSize < HEADER_SIZE || fileSize > MAX_FILE_SIZE)
    return song;

  uint8_t buffer[MAX_FILE_SIZE];
  int bytesRead = Brain.SDcard.loadfile(file, buffer, fileSize);
  if (bytesRead < HEADER_SIZE)
    return song;

  int tempo = readInt32(buffer);
  int count = readInt32(buffer + 4);
  if (tempo <= 0 || count < 0)
    return song;

  int available = (bytesRead - HEADER_SIZE) / NOTE_SIZE;
  if (count > available)
    count = available;
  if (count > MAX_NOTES)
    count = MAX_NOTES;

  song.tempo = tempo;
  for (int i = 0; i < count; i++) {
    int octave = buffer[HEADER_SIZE + i * NOTE_SIZE];
    int pitch = buffer[HEADER_SIZE + i * NOTE_SIZE + 1];
    int length = buffer[HEADER_SIZE + i * NOTE_SIZE + 2];
    if (!validOctave(octave) || !validPitch(pitch) || !validLength(length))
      break; // stop at bad data

    song.notes[i].octave = octave;
    song.notes[i].pitch = (Pitch)pitch;
    song.notes[i].length = (Length)length;
    song.duration++;
  }
  return song;
}
