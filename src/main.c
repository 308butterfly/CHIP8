#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include "SDL2/SDL.h"
#include "chip8.h"
#include "chip8keyboard.h"

#define EXIT_SUCCESS 0

const char keyboard_map[CHIP8_TOTAL_KEYS] = {
  SDLK_0,
  SDLK_1,
  SDLK_2,
  SDLK_3,
  SDLK_4,
  SDLK_5,
  SDLK_6,
  SDLK_7,
  SDLK_8,
  SDLK_9,
  SDLK_a,
  SDLK_b,
  SDLK_c,
  SDLK_d,
  SDLK_e,
  SDLK_f,
};

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    printf("Please provide file to load\n");
    return -1;
  }

  const char* filename = argv[1];
  printf("File to load is: %s\n", filename);

  FILE* f = fopen(filename, "rb");

  if (!f)
  {
    printf("ERROR: Failed to open file.\n");
    fclose(f); // To close the file
    return -1;
  }

  fseek(f, 0, SEEK_END); // go to end of the file
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char buf[size]; // may have problems with this
  int res = fread(buf, size, 1, f);
  if (res != 1)
  {
    printf("Failed to read from file\n");
    fclose(f);
    return -1;
  }

  // For debugging with love
  printf("%s\n", buf);

  struct chip8 chip8;
  chip8_init(&chip8);
  chip8_load(&chip8, buf, size);

  // screen test

  chip8_screen_draw_sprite(&chip8.screen, 62, 10, &chip8.memory.memory[0x14], 5);


  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_Window* window = SDL_CreateWindow(
    EMULATOR_WINDOW_TITLE,
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    CHIP8_WIDTH * CHIP8_WINDOW_SCALE_UP_FACTOR,
    CHIP8_HEIGHT * CHIP8_WINDOW_SCALE_UP_FACTOR,
    SDL_WINDOW_SHOWN
  );

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_TEXTUREACCESS_TARGET);

  while (true)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
      case SDL_QUIT:
      {
        goto out;
        break;
      }
      case SDL_KEYDOWN:
      {
        char key = event.key.keysym.sym;
        int vkey = chip8_keyboard_map(keyboard_map, key);
        if (vkey != -1)
        {
          chip8_keyboard_down(&chip8.keyboard, vkey);
        }
      }
      break;

      case SDL_KEYUP:
      {
        char key = event.key.keysym.sym;
        int vkey = chip8_keyboard_map(keyboard_map, key);
        if (vkey != -1)
        {
          chip8_keyboard_up(&chip8.keyboard, vkey);
        }
      }
      break;

      default:
        break;
      }

    }

    // renders color black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer); // paint over entire screen
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0); // set to white

    for (int x = 0; x < CHIP8_WIDTH; ++x)
    {
      for (int y = 0; y < CHIP8_HEIGHT; ++y)
      {
        if (chip8_screen_is_set(&chip8.screen, x, y))
        {
          SDL_Rect r;
          r.x = x * CHIP8_WINDOW_SCALE_UP_FACTOR;
          r.y = y * CHIP8_WINDOW_SCALE_UP_FACTOR;
          r.w = CHIP8_WINDOW_SCALE_UP_FACTOR;
          r.h = CHIP8_WINDOW_SCALE_UP_FACTOR;
          SDL_RenderFillRect(renderer, &r);

        }

      }
    }

    SDL_RenderPresent(renderer);

    // There is probably a better way to do this.
    if (chip8.registers.delay_timer > 0)
    {
      Sleep(100); // why 100 why not?
      chip8.registers.delay_timer -= 1;
    }

    if (chip8.registers.sound_timer > 0)
    {
      Beep(1000, 100 * chip8.registers.sound_timer);
      chip8.registers.sound_timer = 0;
    }

    unsigned short opcode = chip8_memory_get_short(&chip8.memory, chip8.registers.PC);
    chip8_exec(&chip8, opcode);
    chip8.registers.PC += 2;
    printf("%x\n", opcode);

  }
  // gotos are usually bad but this case they are okay??
out:
  SDL_DestroyWindow(window);
  return EXIT_SUCCESS;
}