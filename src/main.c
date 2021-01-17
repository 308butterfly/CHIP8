#include <stdio.h>

#include "SDL2/SDL.h"
#include "chip8.h"

#define TRUE 1
#define FALSE 0
#define EXIT_SUCCESS 0

int main(int argc, char** argv)
{
  struct chip8 chip8;
  chip8.registers.SP = 0;

  // stack testing code
  chip8_stack_push(&chip8, 0xff);
  chip8_stack_push(&chip8, 0xaa);

  printf("%x\n", chip8_stack_pop(&chip8));
  printf("%x\n", chip8_stack_pop(&chip8));

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

  while (TRUE)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        goto out;
      }
    }

    // renders color black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer); // paint over entire screen
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0); // set to white
    SDL_Rect r;
    r.x = 0;
    r.y = 0;
    r.w = 40;
    r.h = 40;
    SDL_RenderFillRect(renderer, &r);
    SDL_RenderPresent(renderer);
  }
  // gotos are usually bad but this case they are okay??
out:
  SDL_DestroyWindow(window);
  return EXIT_SUCCESS;
}