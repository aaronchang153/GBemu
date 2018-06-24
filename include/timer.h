#ifndef TIMER_H
#define TIMER_H

#include "common.h"
#include "memory.h"

typedef struct{
    WORD system_counter;
    int timer_counter;
    BYTE div;
    BYTE tima;
    BYTE tma;
    BYTE tac;

    MEMORY *memory;
} TIMER;

TIMER *Timer_Create();

void Timer_SetMemory(TIMER *t, MEMORY *mem);

void Timer_Destroy(TIMER *t);

void Timer_Update(TIMER *t, unsigned int cycles);

void Timer_ResetCounter(TIMER *t);

#endif // TIMER_H