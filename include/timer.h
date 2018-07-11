#ifndef TIMER_H
#define TIMER_H

#include "common.h"
#include "memory.h"

typedef struct{
    int timer_counter;
    union{
        struct{
            BYTE __unused;
            BYTE div;
        };
        WORD system_counter;
    };
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