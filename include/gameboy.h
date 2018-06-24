#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "cpu.h"
#include "memory.h"
#include "timer.h"
#include "interrupt.h"


typedef struct{
    CPU *cpu;
    MEMORY *memory;
    TIMER *timer;
    INTERRUPT_HANDLER *interrupt;
} GAMEBOY;


GAMEBOY *GB_Create();

void GB_Destroy(GAMEBOY *gb);

void GB_LoadGame(GAMEBOY *gb, char *filename);

void GB_Startup(GAMEBOY *gb);

void GB_Update(GAMEBOY *gb);

#endif // GAMEBOY_H