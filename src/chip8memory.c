#include "chip8memory.h"

void chip8_memory_set(struct chip8_memory* memory, int index, unsigned char val);

unsigned char chip8_memory_get(struct chip8_memory* memory, int index);
