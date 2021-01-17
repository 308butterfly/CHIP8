#include <stdio.h>

#include "SDL2/SDL.h"

#define TRUE 1
#define FALSE 0
#define EXIT_SUCCESS 0

int main(int argc, char** argv)
{
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_Window* window = SDL_CreateWindow(
    "Chip8 Window",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    640,
    320,
    SDL_WINDOW_SHOWN
  );

  printf("word..........\n");

  SDL_DestroyWindow(window);
  return EXIT_SUCCESS;
}