#include "chip8.h"
#include <memory.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "SDL2/SDL.h"

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

static char chip8_wait_for_key_press(struct chip8* chip8)
{
  SDL_Event event;
  while (SDL_WaitEvent(&event))
  {
    if (event.type != SDL_KEYDOWN)
    {
      continue;
    }
    char c = event.key.keysym.sym;
    char chip8_key = chip8_keyboard_map(&chip8->keyboard, c);
    if (chip8_key)
    {
      return chip8_key;
    }

  }

  return -1;
}
static void chip8_exec_extended_f(struct chip8* chip8, unsigned short opcode)
{
  unsigned char x = (opcode >> 8) & 0x000f;
  switch (opcode & 0x00ff)
  {
  case 0x07: // LD Vx, DT - Set Vx = delay timer value
    chip8->registers.V[x] = chip8->registers.delay_timer;
    break;
  case 0x0a: // LD Vx, K - Wait for a key press, store its value in Vx
  {
    char pressed_key = chip8_wait_for_key_press(chip8);
    chip8->registers.V[x] = pressed_key;
  }
  break;
  case 0x15: // LD DT, Vx - Set delay timer = Vx
    chip8->registers.delay_timer = chip8->registers.V[x];
    break;
  case 0x18: // LD ST, Vx - Set sound timer to Vx
    chip8->registers.sound_timer = chip8->registers.V[x];
    break;
  case 0x1e: // ADD I, Vx - Set I = I + Vx
    chip8->registers.I += chip8->registers.V[x];
    break;
  case 0x29: // LD F, Vx - Set I = to location of hexidecimal sprite whose cooresponding value = Vx
    // Huh??
    chip8->registers.I = chip8->registers.V[x] * CHIP8_DEFAULT_SPRITE_HEIGHT;
    break;
  case 0x33: // LD B, Vx - Take the decimal value of Vx store the hundreds in I, the tens in I + 1, and the ones in I + 2
  {   unsigned char hundreds = chip8->registers.V[x] / 100;
  unsigned char tens = chip8->registers.V[x] / 10 % 10;
  unsigned char ones = chip8->registers.V[x] % 10;
  chip8_memory_set(&chip8->memory, chip8->registers.I, hundreds);
  chip8_memory_set(&chip8->memory, chip8->registers.I + 1, tens);
  chip8_memory_set(&chip8->memory, chip8->registers.I + 2, ones);
  }
  break;
  case 0x55: // LD [I], Vx - Store registers V0 through Vx into memory starting at addr in I
  {
    for (int i = 0; i <= x; ++i)
    {
      chip8_memory_set(&chip8->memory, chip8->registers.I + i, chip8->registers.V[i]);
    }
  }
  break;
  case 0x65: // LD Vx, [I] - Read values from memory starting at location I into registers V0 through Vx
  {
    for (int i; i <= x; ++i)
    {
      chip8->registers.V[i] = chip8_memory_get(&chip8->memory, chip8->registers.I + i);
    }
  }
  break;
  default:
    break;
  }


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
  unsigned char n = opcode & 0x000f;
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
    srand(clock());
    chip8->registers.V[x] = (rand() % 255) & kk;
    break;
  case 0xd000: // Dxyn - DRW Vx, Vy, nibble - Draw instruction, n bytes are read from addr stored in register I. Bytes are displayed on screen at Vx and Vy.  Sprites are XORed onto existing screen with VF = 1 when pixels are erased.  Otherwise it is set to 0.  Sprites can wrap around to other side of screen if positioned outside of screen coordinates.
  {
    const char* sprite = (const char*)&chip8->memory.memory[chip8->registers.I];
    chip8->registers.V[0x0f] = chip8_screen_draw_sprite(&chip8->screen, chip8->registers.V[x], chip8->registers.V[y], sprite, n);
  }
  // Keyboard opcodes
  case 0xe000:
  {
    switch (opcode & 0x00ff)
    {
    case 0x9e: // Ex9E - SKP Vx: Skip next instruction if key with value in Vx is pressed
      if (chip8_key_is_down(&chip8->keyboard, chip8->registers.V[x]))
      {
        chip8->registers.PC += 2;
      }
      break;
    case 0xa1: // ExA1 - SKNP Vx: Skip next instruction if key with value in in Vx is not pressed 
      if (!chip8_key_is_down(&chip8->keyboard, chip8->registers.V[x]))
      {
        chip8->registers.PC += 2;
      }

      break;
    default:
      break;
    }
  }
  break;

  // Delay timers
  case 0xf000:
    chip8_exec_extended_f(chip8, opcode);
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
