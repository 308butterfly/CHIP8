#ifndef CHIP8REGISTERS_H
#define CHIP8REGISTERS_H

#include "config.h"

struct chip8_registers
{
  unsigned char V[CHIP8_TOTAL_DATA_REGISTERS]; // 8 bit registers VF not used
  unsigned short I; // 16 bit registers
  unsigned char delay_timer;
  unsigned char sound_timer;

  unsigned short PC;
  unsigned char SP;
};

#endif