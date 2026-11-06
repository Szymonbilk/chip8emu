#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the standard font */
static const uint8_t font[FONT_SIZE] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
  0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
  0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
  0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
  0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
  0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
  0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
  0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
  0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
  0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
  0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
  0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
  0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
  0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
  0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
  0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};

static void chip8_reset (struct chip8 *chip8);

/* create a chip8 struct, and provide initial values */
struct chip8 *
chip8_init (void)
{
  struct chip8 *chip8 = malloc (sizeof (struct chip8));
  if (chip8 != NULL)
    {
      chip8_reset (chip8);
    }
  return chip8;
}

/* load a rom from a given file */
bool
chip8_load_rom (struct chip8 *chip8, const char *rom)
{
  FILE *file;

  file = fopen (rom, "rb");
  if (file == NULL)
    {
      return false;
    }

  fread (&chip8->memory[PC_START], 1, MAX_ROM_SIZE, file);

  fclose (file);
  return true;
}

/* free the associated memory for a given chip8 struct */
void
chip8_destroy (struct chip8 *chip8)
{
  free (chip8);
}

/* set all values in the state to default values */
static void
chip8_reset (struct chip8 *chip8)
{
  /* set all values to 0 */
  memset (chip8, 0, sizeof (struct chip8));
  chip8->pc = PC_START;
  memcpy (chip8->memory + FONT_START, font, FONT_SIZE);
}
