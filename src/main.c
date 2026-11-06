#include "chip8.h"
#include <stdio.h>

#define PROG_ERR 1

int
main (int argc, char *argv[])
{
  if (argc < 2)
    {
      fprintf (stderr,
               "No file provided. Please provide the path to a ROM to run.\n");
      return PROG_ERR;
    }

  struct chip8 *chip8;
  chip8 = chip8_init ();
  if (chip8 == NULL)
    {
      fprintf (
          stderr,
          "Could not initialise state struct. Maybe not enough memory?\n");
      return PROG_ERR;
    }

  if (!chip8_load_rom (chip8, argv[1]))
    {
      fprintf (stderr, "Could not load ROM. Maybe invalid file?\n");
      return PROG_ERR;
    }

  chip8_destroy (chip8);
}
