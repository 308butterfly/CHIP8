#ifndef CHIP8STACK_H
#define CHIP8STACK_H

#include "config.h"

// Going to write fxns that expect chip8 to be passed
struct chip8; // forward declare

struct chip8_stack
{
  unsigned short stack[CHIP8_STACK_DEPTH];  // 16 16 bit
};

void chip8_stack_push(struct chip8* chip8, unsigned short val);
unsigned short chip8_stack_pop(struct chip8* chip8);

#endif
