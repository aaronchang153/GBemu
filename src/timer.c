#include "timer.h"

#include <stdlib.h>
#include <string.h>

TIMER *Timer_Create(){
    TIMER *timer = malloc(sizeof(TIMER));
    if(timer != NULL){
        memset(timer, 0, sizeof(TIMER));
    }
    return timer;
}

void Timer_SetMemory(TIMER *t, MEMORY *mem){
    if(t != NULL){
        t->memory = mem;
    }
}

void Timer_Destroy(TIMER *t){
    if(t != NULL){
        free(t);
    }
}

void Timer_Update(TIMER *t, unsigned int cycles){
    t->div = Mem_ReadByte(t->memory, DIV_ADDR);
    t->tima = Mem_ReadByte(t->memory, TIMA_ADDR);
    t->tma = Mem_ReadByte(t->memory, TMA_ADDR);
    t->tac = Mem_ReadByte(t->memory, TAC_ADDR);

    if(t->div != (t->system_counter & 0xF0) >> 8){
        t->system_counter = 0;
    }
    t->system_counter += cycles;
    t->div = (t->system_counter & 0xF0) >> 8;
    if(TEST_BIT(t->tac, 2)){ // Bit 2 of TAC is the timer enable
        t->timer_counter -= cycles;
        if(t->timer_counter <= 0){
            if(t->tima == 0xFF){
                t->tima = t->tma;
                /********** Request Interrupt **********/
            }
            else{
                t->tima++;
            }
            Timer_ResetCounter(t);
        }
    }

    Mem_ForceWrite(t->memory, DIV_ADDR, t->div);
    Mem_WriteByte(t->memory, TIMA_ADDR, t->tima);
    Mem_WriteByte(t->memory, TMA_ADDR, t->tma);
    Mem_WriteByte(t->memory, TAC_ADDR, t->tac);
}

void Timer_ResetCounter(TIMER *t){
    switch(Mem_ReadByte(t->memory, TAC_ADDR) & 0x03){
        case 0: // 4096   Hz
            t->timer_counter = 1024;
            break;
        case 1: // 262144 Hz
            t->timer_counter = 16;
            break;
        case 2: // 65536  Hz
            t->timer_counter = 64;
            break;
        case 3: // 16382 Hz
            t->timer_counter = 256;
            break;
    };
}