#include "chip8.h"
#include <inttypes.h>
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

/* fetch, decode and execute one instruction */
void
chip8_step (struct chip8 *chip8)
{
  uint16_t opcode
      = (chip8->memory[chip8->pc] << 8) | chip8->memory[chip8->pc + 1];
  chip8->pc += 2;

  uint8_t vx
      = (opcode & 0x0F00) >> 8; /* second nibble, used for VX register */
  uint8_t vy = (opcode & 0x00F0) >> 4; /* third nibble, used for VY register */
  uint8_t n = (opcode & 0x000F);       /* fourth nibble, a 4-bit number */
  uint8_t nn = (opcode & 0x00FF); /* second byte, an 8-bit immediate number */
  uint16_t nnn = (opcode & 0x0FFF); /* second, third and fourth nibbles, a
                                      12-bit immediate memory address */

  uint8_t flag;

  /* first search based on the first nibble */
  switch (opcode & 0xF000)
    {
      case 0x0000:
        switch (nn)
          {
            case 0x00E0:
              /* clear the display */
              memset (&chip8->display, 0, DISPLAY_SIZE);
              chip8->display_changed = true;
              break;
            case 0x00EE:
              /* return from a subroutine */
              chip8->sp--;
              chip8->pc = chip8->stack[chip8->sp];
              break;
            default:
              goto unknowninstr;
          }
        break;

      case 0x1000:
        /* jump to NNN */
        chip8->pc = nnn;
        break;

      case 0x2000:
        /* call subroutine at NNN */
        chip8->stack[chip8->sp] = chip8->pc;
        chip8->sp++;
        chip8->pc = nnn;
        break;

      case 0x3000:
        /* skip one instruction if VX == NN */
        if (chip8->registers[vx] == nn)
          {
            chip8->pc += 2;
          }
        break;

      case 0x4000:
        /* skip one instruction if VX != NN */
        if (chip8->registers[vx] != nn)
          {
            chip8->pc += 2;
          }
        break;

      case 0x5000:
        /* skip one instruction if VX == VY */
        if (chip8->registers[vx] == chip8->registers[vy])
          {
            chip8->pc += 2;
          }
        break;

      case 0x6000:
        /* set VX to NN */
        chip8->registers[vx] = nn;
        break;

      case 0x7000:
        /* add NN to VX */
        chip8->registers[vx] += nn;
        break;

      case 0x8000:
        switch (n)
          {
            case 0x0000:
              /* VX = VY */
              chip8->registers[vx] = chip8->registers[vy];
              break;
            case 0x0001:
              /* VX = VX | VY */
              chip8->registers[vx] |= chip8->registers[vy];
              break;
            case 0x0002:
              /* VX = VX & VY */
              chip8->registers[vx] &= chip8->registers[vy];
              break;
            case 0x0003:
              /* VX = VX ^ VY */
              chip8->registers[vx] ^= chip8->registers[vy];
              break;
            case 0x0004:
              /* VX = VX + VY, setting carry flag in case of overflow */
              uint16_t result = chip8->registers[vx] + chip8->registers[vy];
              chip8->registers[vx] = result & 0xFF;
              chip8->registers[VF] = result > 0xFF;
              break;
            case 0x0005:
              /* VX = VX - VY, setting carry flag to opposite to case of
               * underflow */
              flag = chip8->registers[vx] >= chip8->registers[vy];
              chip8->registers[vx] -= chip8->registers[vy];
              chip8->registers[VF] = flag;
              break;
            case 0x0006:
              /* NOTE: ambiguous instruction */
              /* VX = VX >> 1, store underflow bit in VF */
              flag = chip8->registers[vx] & 0x1;
              chip8->registers[vx] >>= 1;
              chip8->registers[VF] = flag;
              break;
            case 0x0007:
              /* VX = VY - VX, setting carry flag to opposite to case of
               * underflow */
              flag = chip8->registers[vy] >= chip8->registers[vx];
              chip8->registers[vx]
                  = chip8->registers[vy] - chip8->registers[vx];
              chip8->registers[VF] = flag;
              break;
            case 0x000E:
              /* NOTE: ambiguous instruction */
              /* VX = VX << 1, store overflow bit in VF */
              flag = chip8->registers[vx] >> 7;
              chip8->registers[vx] <<= 1;
              chip8->registers[VF] = flag;
              break;
            default:
              goto unknowninstr;
          }
        break;

      case 0x9000:
        /* skip one instruction if VX != VY */
        if (chip8->registers[vx] != chip8->registers[vy])
          {
            chip8->pc += 2;
          }
        break;

      case 0xA000:
        /* set index to NNN */
        chip8->index = nnn;
        break;

      case 0xB000:
        /* NOTE: ambiguous instruction */
        /* jump to address at NNN + V0 */
        chip8->pc = chip8->registers[0] + nnn;
        break;

      case 0xC000:
        /* set VX to random number & NN */
        chip8->registers[vx] = random () & nn;
        break;

      case 0xD000:
        /* sprite vx vy N */
        uint8_t x_coord = chip8->registers[vx] % DISPLAY_WIDTH;
        uint8_t y_coord = chip8->registers[vy] % DISPLAY_HEIGHT;
        uint8_t row;
        chip8->registers[VF] = 0;

        for (int dy = 0; dy < n; dy++)
          {
            /* clip if we get to the end of the display */
            if (y_coord + dy >= DISPLAY_HEIGHT)
              {
                break;
              }

            /* get a row of pixels from memory */
            row = chip8->memory[chip8->index + dy];

            for (int dx = 0; dx < 8; dx++)
              {
                /* clip if we get to the end of the display */
                if (x_coord + dx >= DISPLAY_WIDTH)
                  {
                    break;
                  }

                /* extract each pixel from the row */
                if ((row & (0x80 >> dx)) != 0)
                  {
                    int display_coord
                        = x_coord + dx + (y_coord + dy) * DISPLAY_WIDTH;

                    if (chip8->display[display_coord] == 1)
                      {
                        chip8->registers[VF] = 1;
                      }

                    chip8->display[display_coord] ^= 1;
                  }
              }
          }

        chip8->display_changed = true;
        break;

      case 0xE000:
        switch (nn)
          {
            case 0x009E:
              /* skip one instruction if key VX is pressed */
              if (chip8->keypad[chip8->registers[vx]])
                {
                  chip8->pc += 2;
                }
              break;
            case 0x00A1:
              /* skip one instruction if key VX is not pressed */
              if (!chip8->keypad[chip8->registers[vx]])
                {
                  chip8->pc += 2;
                }
              break;
            default:
              goto unknowninstr;
          }
        break;

      case 0xF000:
        switch (nn)
          {
            case 0x0007:
              /* set VX to the delay timer */
              chip8->registers[vx] = chip8->delay_timer;
              break;
            case 0x000A:
              /* block until any key is pressed and store it into VX */
              bool found = false;

              for (int k = 0; k < NUM_KEYS; k++)
                {
                  if (chip8->keypad[k])
                    {
                      found = true;
                      break;
                    }
                }

              if (!found)
                {
                  chip8->pc -= 2;
                }
              break;
            case 0x0015:
              /* set the delay timer to VX */
              chip8->delay_timer = chip8->registers[vx];
              break;
            case 0x0018:
              /* set the sound timer to VX */
              chip8->sound_timer = chip8->registers[vx];
              break;
            case 0x001E:
              /* NOTE: ambiguous instruction */
              /* add VX to index register */
              uint16_t result = chip8->index + chip8->registers[vx];
              chip8->index = result & 0xFFF;
              chip8->registers[VF] = result > 0xFFF;
              break;
            case 0x0029:
              /* set index register to font memory address of character in VX
               */
              chip8->index = chip8->registers[vx] * CHAR_SIZE + FONT_START;
              break;
            case 0x0033:
              /* convert VX to BCD, stored in memory starting at index register
               */
              uint8_t vx_val = chip8->registers[vx];
              chip8->memory[chip8->index] = vx_val / 100;
              chip8->memory[chip8->index + 1] = (vx_val / 10) % 10;
              chip8->memory[chip8->index + 2] = vx_val % 10;
              break;
            case 0x0055:
              /* NOTE: ambiguous instruction */
              /* store registers from V0 to VX inclusive in memory */
              for (int i = 0; i <= vx; i++)
                {
                  chip8->memory[chip8->index + i] = chip8->registers[i];
                }
              break;
            case 0x0065:
              /* NOTE: ambiguous instruction */
              /* read memory into register V0 to VX inclusive */
              for (int i = 0; i <= vx; i++)
                {
                  chip8->registers[i] = chip8->memory[chip8->index + i];
                }
              break;
            default:
              goto unknowninstr;
          }
        break;

      default:
      unknowninstr:
        fprintf (stderr, "Unrecognised instruction: %" PRIx16 "\n", opcode);
        exit (1);
    }
}

/* decrement the timers if they are above 0 */
void
chip8_timer_tick (struct chip8 *chip8)
{
  if (chip8->delay_timer > 0)
    {
      chip8->delay_timer--;
    }
  if (chip8->sound_timer > 0)
    {
      chip8->sound_timer--;
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
  chip8->display_changed = true;
  memcpy (chip8->memory + FONT_START, font, FONT_SIZE);
}
