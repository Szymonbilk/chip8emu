#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define INIT_SCALE 12

#define CPU_HZ 700
#define TIMER_HZ 60
#define CPU_TIME_SLICE (SDL_NS_PER_SECOND / CPU_HZ)
#define TIMER_TIME_SLICE (SDL_NS_PER_SECOND / TIMER_HZ)
#define MAX_DT (100 * SDL_NS_PER_MS)

/* stores the state of the whole application */
struct app
{
  struct chip8 *chip8; /* the chip8 state */

  SDL_Window *win;   /* the actual window */
  SDL_Renderer *ren; /* what draws into the window */
  SDL_Texture *tex;  /* the image the renderer can draw */

  uint32_t pixels[DISPLAY_SIZE]; /* the pixels to be put into the texture */

  uint64_t last;            /* the time the last frame started */
  uint64_t cpu_time_left;   /* how much time the cpu has to step within the
                               current frame */
  uint64_t timer_time_left; /* how much time the timers have to tick within the
                               current frame */
};

/* the mapping of keyboard keys to the chip8 keypad */
static const SDL_Scancode keymap[NUM_KEYS]
    = { SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
        SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
        SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
        SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V };

/* initialise the application state */
SDL_AppResult
SDL_AppInit (void **appstate, int argc, char **argv)
{
  /* seed random with the current time */
  srand (time (NULL));

  struct app *app = calloc (1, sizeof (struct app));
  if (app == NULL)
    {
      fprintf (stderr, "Out of memory.\n");
      return SDL_APP_FAILURE;
    }
  *appstate = app;

  if (argc < 2)
    {
      fprintf (stderr,
               "No file provided. Please provide the path to a ROM to run.\n");
      return SDL_APP_FAILURE;
    }

  app->chip8 = chip8_init ();
  if (app->chip8 == NULL)
    {
      fprintf (
          stderr,
          "Could not initialise state struct. Maybe not enough memory?\n");
      return SDL_APP_FAILURE;
    }

  if (!chip8_load_rom (app->chip8, argv[1]))
    {
      fprintf (stderr, "Could not load ROM. Maybe invalid file?\n");
      return SDL_APP_FAILURE;
    }

  if (!SDL_Init (SDL_INIT_VIDEO))
    {
      SDL_Log ("SDL_Init: %s", SDL_GetError ());
      return SDL_APP_FAILURE;
    }

  if (!SDL_CreateWindowAndRenderer ("chip8emu", DISPLAY_WIDTH * INIT_SCALE,
                                    DISPLAY_HEIGHT * INIT_SCALE, 0, &app->win,
                                    &app->ren))
    {
      SDL_Log ("Create: %s", SDL_GetError ());
      return SDL_APP_FAILURE;
    }

  app->tex = SDL_CreateTexture (app->ren, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH,
                                DISPLAY_HEIGHT);
  if (app->tex == NULL)
    {
      SDL_Log ("CreateTexture: %s", SDL_GetError ());
      return SDL_APP_FAILURE;
    }
  SDL_SetTextureScaleMode (app->tex, SDL_SCALEMODE_NEAREST);

  /* if we do not have vsync, set the iteration rate to 60 Hz */
  if (!SDL_SetRenderVSync (app->ren, 1))
    {
      SDL_SetHint (SDL_HINT_MAIN_CALLBACK_RATE, "60");
    }

  app->last = SDL_GetTicksNS ();
  return SDL_APP_CONTINUE;
}

/* runs every time an event occurs */
SDL_AppResult
SDL_AppEvent (void *appstate, SDL_Event *event)
{
  struct app *app = appstate;

  if (event->type == SDL_EVENT_QUIT)
    {
      return SDL_APP_SUCCESS;
    }

  if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
    {
      bool down = (event->type == SDL_EVENT_KEY_DOWN);

      for (int k = 0; k < NUM_KEYS; k++)
        {
          if (event->key.scancode == keymap[k])
            {
              app->chip8->keypad[k] = down;
            }
        }
    }

  return SDL_APP_CONTINUE;
}

/* run the core emulator loop every frame */
SDL_AppResult
SDL_AppIterate (void *appstate)
{
  struct app *app = appstate;

  uint64_t frame_start = SDL_GetTicksNS ();
  uint64_t dt = frame_start - app->last;
  app->last = frame_start;
  /* pause the emulator after 100ms of freezing to avoid too many
   * instructions occuring at once for a longer freeze */
  if (dt > MAX_DT)
    {
      dt = MAX_DT;
    }

  app->cpu_time_left += dt;
  while (app->cpu_time_left >= CPU_TIME_SLICE)
    {
      chip8_step (app->chip8);
      app->cpu_time_left -= CPU_TIME_SLICE;
    }
  app->timer_time_left += dt;
  while (app->timer_time_left >= TIMER_TIME_SLICE)
    {
      chip8_timer_tick (app->chip8);
      app->timer_time_left -= TIMER_TIME_SLICE;
    }

  if (app->chip8->display_changed)
    {
      for (int i = 0; i < DISPLAY_SIZE; i++)
        {
          app->pixels[i] = app->chip8->display[i] ? 0xFFFFFFFF : 0xFF000000;
        }

      SDL_UpdateTexture (app->tex, NULL, app->pixels,
                         DISPLAY_WIDTH * sizeof (uint32_t));
      app->chip8->display_changed = false;
    }

  SDL_RenderClear (app->ren);
  SDL_RenderTexture (app->ren, app->tex, NULL,
                     NULL); // NULL dst = stretch to window
  SDL_RenderPresent (app->ren);

  return SDL_APP_CONTINUE;
}

/* cleanup on exit */
void
SDL_AppQuit (void *appstate, SDL_AppResult result)
{
  (void)result;
  struct app *app = appstate;
  if (app == NULL)
    {
      return;
    }

  SDL_DestroyTexture (app->tex);
  SDL_DestroyRenderer (app->ren);
  SDL_DestroyWindow (app->win);
  chip8_destroy (app->chip8);
  free (app);
}
