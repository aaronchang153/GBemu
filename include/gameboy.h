#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "cpu.h"
#include "memory.h"
#include "timer.h"
#include "interrupt.h"
#include "graphics.h"
#include "display.h"
#include "joypad.h"
#include "audio.h"


typedef struct{
    CPU *cpu;
    MEMORY *memory;
    TIMER *timer;
    GRAPHICS *graphics;
    DISPLAY *display;
    JOYPAD *joypad;
    APU *apu;
} GAMEBOY;


GAMEBOY *GB_Create();

void GB_Destroy(GAMEBOY *gb);

void GB_LoadGame(GAMEBOY *gb, char *filename);

void GB_Startup(GAMEBOY *gb);

void GB_Update(GAMEBOY *gb);

#endif // GAMEBOY_H