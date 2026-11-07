#include "chip8.h"
#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define PROG_ERR 1
#define INIT_SCALE 12

#define FRAME_RATE 60
#define CPU_HZ 700
#define TIMER_HZ 60
#define CPU_TIME_SLICE (SDL_NS_PER_SECOND / CPU_HZ)
#define TIMER_TIME_SLICE (SDL_NS_PER_SECOND / TIMER_HZ)
#define FRAME_TIME_SLICE (SDL_NS_PER_SECOND / FRAME_RATE)

int
main (int argc, char *argv[])
{
  int ret = PROG_ERR;
  struct chip8 *chip8 = NULL;
  SDL_Window *win = NULL;
  SDL_Renderer *ren = NULL;
  SDL_Texture *tex = NULL;

  if (argc < 2)
    {
      fprintf (stderr,
               "No file provided. Please provide the path to a ROM to run.\n");
      goto cleanup;
    }

  chip8 = chip8_init ();
  if (chip8 == NULL)
    {
      fprintf (
          stderr,
          "Could not initialise state struct. Maybe not enough memory?\n");
      goto cleanup;
    }

  if (!chip8_load_rom (chip8, argv[1]))
    {
      fprintf (stderr, "Could not load ROM. Maybe invalid file?\n");
      goto cleanup;
    }

  if (!SDL_Init (SDL_INIT_VIDEO))
    {
      SDL_Log ("SDL_Init: %s", SDL_GetError ());
      goto cleanup;
    }

  if (!SDL_CreateWindowAndRenderer ("chip8emu", DISPLAY_WIDTH * INIT_SCALE,
                                    DISPLAY_HEIGHT * INIT_SCALE, 0, &win,
                                    &ren))
    {
      SDL_Log ("Create: %s", SDL_GetError ());
      goto cleanup;
    }
  bool vsync = SDL_SetRenderVSync (ren, 1);

  tex = SDL_CreateTexture (ren, SDL_PIXELFORMAT_ARGB8888,
                           SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH,
                           DISPLAY_HEIGHT);
  if (tex == NULL)
    {
      SDL_Log ("CreateTexture: %s", SDL_GetError ());
      goto cleanup;
    }
  SDL_SetTextureScaleMode (tex, SDL_SCALEMODE_NEAREST);

  uint32_t pixels[DISPLAY_SIZE];
  bool running = true;

  uint64_t last = SDL_GetTicksNS ();
  uint64_t cpu_time_left = 0;
  uint64_t timer_time_left = 0;

  while (running)
    {
      uint64_t frame_start = SDL_GetTicksNS ();
      uint64_t dt = frame_start - last;
      last = frame_start;
      /* pause the emulator after 100ms of freezing to avoid too many
       * instructions occuring at once for a longer freeze */
      if (dt > 100 * SDL_NS_PER_MS)
        {
          dt = 100 * SDL_NS_PER_MS;
        }

      SDL_Event e;
      while (SDL_PollEvent (&e))
        {
          if (e.type == SDL_EVENT_QUIT)
            {
              running = false;
            }
        }

      cpu_time_left += dt;
      while (cpu_time_left >= CPU_TIME_SLICE)
        {
          chip8_step (chip8);
          cpu_time_left -= CPU_TIME_SLICE;
        }
      timer_time_left += dt;
      while (timer_time_left >= TIMER_TIME_SLICE)
        {
          /* tick timers */
          timer_time_left -= TIMER_TIME_SLICE;
        }

      for (int i = 0; i < DISPLAY_SIZE; i++)
        {
          pixels[i] = chip8->display[i] ? 0xFFFFFFFF : 0xFF000000;
        }

      SDL_UpdateTexture (tex, NULL, pixels, DISPLAY_WIDTH * sizeof (uint32_t));
      SDL_RenderClear (ren);
      SDL_RenderTexture (ren, tex, NULL, NULL); // NULL dst = stretch to window
      SDL_RenderPresent (ren);

      if (!vsync)
        {
          uint64_t spent = SDL_GetTicksNS () - frame_start;
          if (spent < FRAME_TIME_SLICE)
            {
              SDL_DelayPrecise (FRAME_TIME_SLICE - spent);
            }
        }
    }
  ret = 0;

cleanup:
  SDL_DestroyTexture (tex);
  SDL_DestroyRenderer (ren);
  SDL_DestroyWindow (win);
  SDL_Quit ();
  chip8_destroy (chip8);
  return ret;
}
