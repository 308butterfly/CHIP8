#include "chip8.h"
#include <memory.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

const char chip8_default_character_set[] = {
  0xf0, 0x90, 0x90, 0x90, 0xf0, //0
  0x20, 0x60, 0x20, 0x20, 0x70, //1
  0xf0, 0x10, 0xf0, 0x80, 0xf0, //2
  0xf0, 0x10, 0xf0, 0x10, 0xf0, //3
  0x90, 0x90, 0xf0, 0x10, 0x10, //4
  0xf0, 0x80, 0xf0, 0x10, 0xf0, //5
  0xf0, 0x80, 0xf0, 0x90, 0xf0, //6
  0xf0, 0x10, 0x20, 0x40, 0x40, //7
  0xf0, 0x90, 0xf0, 0x90, 0xf0, //8
  0xf0, 0x90, 0xf0, 0x10, 0xf0, //9
  0xf0, 0x90, 0xf0, 0x90, 0x90, //A
  0xe0, 0x90, 0xe0, 0x90, 0xe0, //B
  0xf0, 0x80, 0x80, 0x80, 0xf0, //C
  0xe0, 0x90, 0x90, 0x90, 0xe0, //D
  0xf0, 0x80, 0xf0, 0x80, 0xf0, //E
  0xf0, 0x80, 0xf0, 0x80, 0x80, //F
};

void chip8_init(struct chip8* chip8)
{
  memset(chip8, 0, sizeof(struct chip8));
  memcpy(&chip8->memory.memory, chip8_default_character_set, sizeof(chip8_default_character_set));
}

void chip8_load(struct chip8* chip8, const char* buf, size_t size)
{
  assert(size + CHIP8_PROGRAM_LOAD_ADDR < CHIP8_MEMORY_SIZE);
  memcpy(&chip8->memory.memory[CHIP8_PROGRAM_LOAD_ADDR], buf, size);
  chip8->registers.PC = CHIP8_PROGRAM_LOAD_ADDR;
}

static void chip8_exec_extended_eight(struct chip8* chip8, unsigned short opcode)
{
  unsigned char x = (opcode >> 8) & 0x000f;
  unsigned char y = (opcode >> 4) & 0x000f;
  unsigned char final_four_bits = opcode & 0x000f;
  unsigned short tmp = 0;

  switch (final_four_bits)
  {
  case 0x00: // LD Vx, Vy : Set Vx = Vy
    chip8->registers.V[x] = chip8->registers.V[y];
    break;
  case 0x01: // OR Vx, Vy : Set Vx = Vx OR Vy
    chip8->registers.V[x] = chip8->registers.V[x] | chip8->registers.V[y];
    break;
  case 0x02: // AND Vx, Vy: Set Vx = Vx AND Vy
    chip8->registers.V[x] = chip8->registers.V[x] & chip8->registers.V[y];
    break;
  case 0x03: // XOR Vx, Vy: Set Vx = Vx XOR Vy
    chip8->registers.V[x] = chip8->registers.V[x] ^ chip8->registers.V[y];
    break;
  case 0x04: // ADD Vx, Vy: Set Vx = Vx + Vy, set VF = carry if > 255
             // Only lowest 8 bits are kept and stored in Vx 
    tmp = chip8->registers.V[x] + chip8->registers.V[y];
    chip8->registers.V[0x0f] = false;
    if (tmp > 0xff)
    {
      chip8->registers.V[0x0f] = true;
    }
    chip8->registers.V[x] = tmp;
    break;
  case 0x05:  // SUB Vx Vy: Set Vx = Vx - Vy, if Vx > Vy then VF = 1 otherwise 0

    chip8->registers.V[x] -= chip8->registers.V[y];
    chip8->registers.V[0x0f] = chip8->registers.V[x] > chip8->registers.V[y];

    break;
  case 0x06: // SHR Vx {, Vy} If least significant bit of Vx = 1 then VF = 1, otherwise 0.  Vx is then divided by two
    chip8->registers.V[0x0f] = chip8->registers.V[x] & 0x01;
    chip8->registers.V[x] /= 2;
    break;
  case 0x07: // SUBN Vx, Vy: If Vy > Vx, then VF = 1, otherwise 0.  Then Vx is subtracted from Vy and the results are stored in Vx
    chip8->registers.V[0x0f] = chip8->registers.V[x] < chip8->registers.V[y];
    chip8->registers.V[x] = chip8->registers.V[y] - chip8->registers.V[x];

    break;
  case 0x0E: // SHL Vx {, Vy}: If most significant bit of Vx = 1 then VF = 1, otherwise 0.  Next Vx is multiplied by two
    chip8->registers.V[0x0f] = chip8->registers.V[x] & 0x80;
    chip8->registers.V[x] *= 2;
    break;
  default:
    break;
  }

}



static void chip8_exec_extended(struct chip8* chip8, unsigned short opcode)
{
  unsigned short nnn = opcode & 0x0fff;
  unsigned char x = (opcode >> 8) & 0x000f;
  unsigned char y = (opcode >> 4) & 0x000f;
  unsigned char kk = opcode & 0x00ff;

  switch (opcode & 0xf000)
  {
    // JP addr, Jump to location nnns
  case 0x1000:
    chip8->registers.PC = nnn;
    break;
  case 0x2000: // CALL addr
    chip8_stack_push(chip8, chip8->registers.PC);
    chip8->registers.PC = nnn;
    break;
  case 0x3000: // SE Vx, byte: 3xkk Skip next instruction if Vx == kk
    if (chip8->registers.V[x] == kk)
    {
      chip8->registers.PC += 2;
    }
    break;
  case 0x4000: // SNE Vx, byte: 4xkk Skip next instruction if Vx != kk
    if (chip8->registers.V[x] != kk)
    {
      chip8->registers.PC += 2;
    }
  case 0x5000: // SE Vx, Vy: 5xy0 Skip next instruction if Vx == Vy
    if (chip8->registers.V[x] == chip8->registers.V[y])
    {
      chip8->registers.PC += 2;
    }
    break;
  case 0x6000: // LD, Vx, byte: 6xkk Load kk into register Vx
    chip8->registers.V[x] = kk;
    break;
  case 0x7000: // ADD Vx, byte: 7xkk Add kk to register Vx and store
    chip8->registers.V[x] += kk;
    break;
  case 0x8000: // for 8000 series instructions
    chip8_exec_extended_eight(chip8, opcode);
    break;
  case 0x9000: // SNE Vx, Vy:  If Vx != Vy the next instruction is skipped
    if (chip8->registers.V[x] != chip8->registers.V[y])
    {
      chip8->registers.PC += 2;
    }
    break;
  case 0xa000: // Annn - LD I, addr: Register I is set to nnn;
    chip8->registers.I = nnn;
    break;
  case 0xb000: // Bnnn - JP V0, addr: Program counter is set to value of V0
    chip8->registers.PC = nnn + chip8->registers.V[0x00];
    break;
  case 0xc000: // Cxkk - RND Vx, byte: And kk with a random number between 0 and 255.  Store in Vx
    break;
  default:
    break;
  }
}

void chip8_exec(struct chip8* chip8, unsigned short opcode)
{
  switch (opcode)
  {
  case 0x00e0:  // CLS: clear screen
    chip8_screen_clear(&chip8->screen);
    break;
  case 0x00ee:  // RET: return from subroutine
    chip8->registers.PC = chip8_stack_pop(chip8);
    break;
  default:
    chip8_exec_extended(chip8, opcode);

  }
}
