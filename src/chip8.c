#include "chip8.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

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

/* fetch, decode and execute one instruction */
void
chip8_step (struct chip8 *chip8)
{
  uint16_t opcode
      = (chip8->memory[chip8->pc++] << 8) | chip8->memory[chip8->pc++];

  uint8_t vx
      = (opcode & 0x0F00) >> 8; /* second nibble, used for VX register */
  uint8_t vy = (opcode & 0x00F0) >> 4; /* third nibble, used for VY register */
  uint8_t n = (opcode & 0x000F);       /* fourth nibble, a 4-bit number */
  uint8_t nn = (opcode & 0x00FF);  /* second byte, an 8-bit immediate number */
  uint8_t nnn = (opcode & 0x0FFF); /* second, third and fourth nibbles, a
                                      12-bit immediate memory address */

  /* first search based on the first nibble */
  switch (opcode & 0xF000)
    {
      case 0x0000:
        switch (opcode & 0x00FF)
          {
            /* clear the display */
            case 0x00E0:
              memset (&chip8->memory, 0, DISPLAY_SIZE);
              break;
            /* return from a subroutine */
            case 0x00EE:
              break;
          }
        break;
      case 0x1000:
        /* jump to NNN */
        chip8->pc = nnn;
        break;
      case 0x6000:
        /* set vx to NN */
        chip8->registers[vx] = nn;
        break;
      case 0x7000:
        /* add NN to vx */
        chip8->registers[vx] += nn;
        break;
      case 0xA000:
        /* set index to NNN */
        chip8->index = nnn;
        break;
      case 0xD000:
        /* sprite vx vy N */
        uint8_t x_coord = chip8->registers[vx] % DISPLAY_WIDTH;
        uint8_t y_coord = chip8->registers[vy] % DISPLAY_HEIGHT;
        uint8_t row;
        chip8->registers[VF] = 0;

        int sprite_y_end = y_coord + n - 1;
        int sprite_x_end = x_coord + 8 - 1;
        int y_end = MIN (DISPLAY_HEIGHT, sprite_y_end);
        int x_end = MIN (DISPLAY_WIDTH, sprite_x_end);

        for (int dy = 0; dy < y_end; dy++)
          {
            /* get a row of pixels from memory */
            row = chip8->memory[chip8->index + dy];

            for (int dx = 0; dx < x_end; dx++)
              {
                /* extract each pixel from the row */
                if ((row & (0x80 >> dx)) != 0)
                  {
                    int display_coord
                        = x_coord + dx + (y_coord + dy) * DISPLAY_HEIGHT;

                    if (chip8->display[display_coord] == 1)
                      {
                        chip8->registers[VF] = 1;
                      }

                    chip8->display[display_coord] ^= 1;
                  }
              }
          }
        break;
      default:
        fprintf (stderr, "Unrecognised instruction: %" PRIx16 "\n", opcode);
        exit (1);
    }
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
