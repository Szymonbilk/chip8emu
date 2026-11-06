#pragma once

#include <stdint.h>

#define MEM_SIZE 4096
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define DISPLAY_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT)
#define NUM_REG 16
#define STACK_SIZE 16
#define NUM_KEYS 16

#define PC_START 0x200
#define FONT_START 0x050
#define FONT_SIZE (5 * 16)

/* struct representing the chip8 system */
struct chip8
{
  uint8_t memory[MEM_SIZE]; /* 4KB of system memory */

  uint8_t registers[NUM_REG]; /* the 16 general-purpose registers (VF can be
                                 used for flags by some operations) */
  uint16_t pc;                /* program counter */
  uint16_t index; /* index register, points to locations in memory */

  uint16_t stack[STACK_SIZE]; /* a stack of depth 16 */
  uint8_t sp;                 /* stack pointer */

  uint8_t
      delay_timer; /* timer which is decremented at 60 Hz until reaching 0 */
  uint8_t sound_timer; /* similar to delay_timer, but beeps when not 0 */

  uint8_t keypad[NUM_KEYS];       /* the hex input keypad */
  uint32_t display[DISPLAY_SIZE]; /* the monochrom output display */
};

struct chip8 *chip8_init (void);
