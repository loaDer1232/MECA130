#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "music.h"

#define BUFFER_SIZE 128
#define MAX_NOTES 256
#define HEADER_SIZE 8
#define NOTE_SIZE 3

static const int MAX_FILE_SIZE = HEADER_SIZE + MAX_NOTES * NOTE_SIZE;

// Prompts until the user enters a whole number in [min, max].
static int getInteger(const char *prompt, int min, int max) {
  char response[BUFFER_SIZE];
  while (true) {
    printf("%s\n", prompt);
    fflush(stdout);
    if (fgets(response, BUFFER_SIZE, stdin) == NULL)
      exit(EXIT_FAILURE);

    char *end;
    long value = strtol(response, &end, 10);
    if (end != response && value >= min && value <= max)
      return (int)value;
    printf("Enter a whole number from %d to %d.\n", min, max);
  }
}

static bool charToPitch(char c, Pitch *out) {
  switch (toupper((unsigned char)c)) {
  case 'C':
    *out = C;
    return true;
  case 'D':
    *out = D;
    return true;
  case 'E':
    *out = E;
    return true;
  case 'F':
    *out = F;
    return true;
  case 'G':
    *out = G;
    return true;
  case 'A':
    *out = A;
    return true;
  case 'B':
    *out = B;
    return true;
  case '_':
    *out = REST;
    return true;
  default:
    return false;
  }
}

// Prompts until the user enters a valid note letter A-G.
static Pitch getPitch(const char *prompt) {
  char response[BUFFER_SIZE];
  Pitch pitch;
  while (true) {
    printf("%s\n", prompt);
    fflush(stdout);
    if (fgets(response, BUFFER_SIZE, stdin) == NULL)
      exit(EXIT_FAILURE);
    if (response[0] != '\0' && charToPitch(response[0], &pitch))
      return pitch;
    printf("Enter a note from A to G or _ for rest.\n");
  }
}

static Length getLength(const char *prompt) {
  while (true) {
    int value = getInteger(prompt, 1, 8);
    if (value == WHOLE || value == HALF || value == QUARTER || value == EIGHTH)
      return (Length)value;
    printf("Length must be 1, 2, 4 or 8.\n");
  }
}

static void putInt32(uint8_t *p, int v) {
  p[0] = v & 0xFF;
  p[1] = (v >> 8) & 0xFF;
  p[2] = (v >> 16) & 0xFF;
  p[3] = (v >> 24) & 0xFF;
}

static int readInt32(const uint8_t *p) {
  return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static bool validOctave(int o) { return o >= MIN_OCTAVE && o <= MAX_OCTAVE; }

static bool validPitch(int p) {
  return p == C || p == D || p == E || p == F || p == G || p == A || p == B ||
         p == REST;
}

static bool validLength(int l) {
  return l == WHOLE || l == HALF || l == QUARTER || l == EIGHTH;
}

// Format: 4-byte LE tempo, 4-byte LE note count, then 3 bytes per note
// (octave, pitch semitone, length). Matches readSong on the robot.
int writeSongToFile(const Song *song, const char *fileName) {
  uint8_t buffer[MAX_FILE_SIZE];
  size_t size = HEADER_SIZE + (size_t)song->duration * NOTE_SIZE;

  putInt32(buffer, song->tempo);
  putInt32(buffer + 4, song->duration);
  for (int i = 0; i < song->duration; i++) {
    buffer[HEADER_SIZE + i * NOTE_SIZE] = (uint8_t)song->notes[i].octave;
    buffer[HEADER_SIZE + i * NOTE_SIZE + 1] = (uint8_t)song->notes[i].pitch;
    buffer[HEADER_SIZE + i * NOTE_SIZE + 2] = (uint8_t)song->notes[i].length;
  }

  FILE *file = fopen(fileName, "wb");
  if (file == NULL) {
    perror("Error opening file");
    return EXIT_FAILURE;
  }

  int result = EXIT_SUCCESS;
  if (fwrite(buffer, 1, size, file) != size) {
    printf("Error writing song data to file.\n");
    result = EXIT_FAILURE;
  } else {
    printf("Song saved to %s (%zu bytes).\n", fileName, size);
  }
  fclose(file);
  return result;
}

Song compose() {
  Song song = {};
  song.tempo = getInteger("Tempo (BPM):", 1, 1000);
  song.duration = getInteger("Note Count:", 1, MAX_NOTES);

  for (int i = 0; i < song.duration; i++) {
    printf("--- Note %d of %d ---\n", i + 1, song.duration);
    song.notes[i].octave = getInteger("Octave:", MIN_OCTAVE, MAX_OCTAVE);
    song.notes[i].pitch = getPitch("Pitch (A-G):");
    song.notes[i].length =
        getLength("Length (1=whole, 2=half, 4=quarter, 8=eighth):");
  }
  return song;
}

// Returns true on success; prints the reason and returns false otherwise.
bool readSongFromFile(const char *fileName, Song *song) {
  FILE *file = fopen(fileName, "rb");
  if (file == NULL) {
    perror("Error opening file");
    return false;
  }

  unsigned char buffer[MAX_FILE_SIZE] = {0};
  size_t bytesRead = fread(buffer, 1, sizeof(buffer), file);
  fclose(file); // done with the file; every path below is safe

  if (bytesRead < HEADER_SIZE) {
    printf("File too small to be a song.\n");
    return false;
  }

  int tempo = readInt32(buffer);
  int count = readInt32(buffer + 4);
  int available = ((int)bytesRead - HEADER_SIZE) / NOTE_SIZE;
  if (tempo <= 0 || count < 0 || count > available || count > MAX_NOTES) {
    printf("Invalid song header (tempo %d, count %d).\n", tempo, count);
    return false;
  }

  *song = Song{};
  song->tempo = tempo;
  for (int i = 0; i < count; i++) {
    int octave = buffer[HEADER_SIZE + i * NOTE_SIZE];
    int pitch = buffer[HEADER_SIZE + i * NOTE_SIZE + 1];
    int length = buffer[HEADER_SIZE + i * NOTE_SIZE + 2];
    if (!validOctave(octave) || !validPitch(pitch) || !validLength(length)) {
      printf("Bad note data at note %d.\n", i + 1);
      return false;
    }
    song->notes[i].octave = octave;
    song->notes[i].pitch = (Pitch)pitch;
    song->notes[i].length = (Length)length;
    song->duration++;
  }
  return true;
}

// Reads the text format that printSong produces:
//   Tempo: 120 BPM, 3 notes
//     1: C4  1/4
//     2: E4  1/4
// Blank lines and lines starting with '#' are ignored. The note numbers
// are ignored too, so you can hand-write or reorder lines freely.
bool readSongFromText(const char *fileName, Song *song) {
  FILE *file = fopen(fileName, "r");
  if (file == NULL) {
    perror("Error opening file");
    return false;
  }

  *song = Song{};
  char line[BUFFER_SIZE];
  bool haveHeader = false;
  int declared = 0;
  int lineNo = 0;

  while (fgets(line, sizeof(line), file) != NULL) {
    lineNo++;
    const char *p = line;
    while (isspace((unsigned char)*p))
      p++;
    if (*p == '\0' || *p == '#')
      continue;

    if (!haveHeader) {
      if (sscanf(p, "Tempo: %d BPM, %d notes", &song->tempo, &declared) != 2 ||
          song->tempo <= 0) {
        printf("Line %d: expected 'Tempo: <bpm> BPM, <n> notes'.\n", lineNo);
        fclose(file);
        return false;
      }
      haveHeader = true;
      continue;
    }

    if (song->duration >= MAX_NOTES) {
      printf("Line %d: too many notes (max %d).\n", lineNo, MAX_NOTES);
      fclose(file);
      return false;
    }

    char letter;
    int octave, length;
    Pitch pitch;
    if (sscanf(p, "%*d: %c%d 1/%d", &letter, &octave, &length) != 3 ||
        !charToPitch(letter, &pitch) || !validOctave(octave) ||
        !validLength(length)) {
      printf("Line %d: expected a note like '1: C4  1/4'.\n", lineNo);
      fclose(file);
      return false;
    }

    Note &note = song->notes[song->duration++];
    note.octave = octave;
    note.pitch = pitch;
    note.length = (Length)length;
  }
  fclose(file);

  if (!haveHeader || song->duration == 0) {
    printf("No song found in %s.\n", fileName);
    return false;
  }
  if (declared != song->duration) {
    printf("Warning: header says %d notes, found %d. Using %d.\n", declared,
           song->duration, song->duration);
  }
  return true;
}

static const char *pitchName(Pitch p) {
  switch (p) {
  case C:
    return "C";
  case D:
    return "D";
  case E:
    return "E";
  case F:
    return "F";
  case G:
    return "G";
  case A:
    return "A";
  case B:
    return "B";
  case REST:
    return "_";
  }
  return "?";
}

void printSong(const Song *song) {
  printf("Tempo: %d BPM, %d notes\n", song->tempo, song->duration);
  for (int i = 0; i < song->duration; i++) {
    printf("%3d: %s%d  1/%d\n", i + 1, pitchName(song->notes[i].pitch),
           song->notes[i].octave, (int)song->notes[i].length);
  }
}

int main(int argc, char *argv[]) {
  const char *fileName = (argc > 1) ? argv[1] : "song.vmf";
  const char *mode = (argc > 2) ? argv[2] : "comp";

  if (strcmp(mode, "comp") == 0) {
    Song song = compose();
    return writeSongToFile(&song, fileName);
  }

  if (strcmp(mode, "print") == 0) {
    Song song = {};
    if (!readSongFromFile(fileName, &song))
      return EXIT_FAILURE;
    printSong(&song);
    return EXIT_SUCCESS;
  }

  if (strcmp(mode, "import") == 0) {
    if (argc < 4) {
      fprintf(stderr, "Usage: %s out.vmf import song.txt\n", argv[0]);
      return EXIT_FAILURE;
    }
    Song song = {};
    if (!readSongFromText(argv[3], &song))
      return EXIT_FAILURE;
    return writeSongToFile(&song, fileName);
  }

  fprintf(stderr, "Unknown mode '%s'. Use 'comp', 'print' or 'import'.\n",
          mode);
  return EXIT_FAILURE;
}
