#ifndef CPU_H
#define CPU_H

#include "common.h"
#include "memory.h"

#define Z_FLAG 0x80
#define N_FLAG 0x40
#define H_FLAG 0x20
#define C_FLAG 0x10

typedef struct{
    WORD pc;
    WORD sp;
    BYTE a, f; 
    BYTE b, c;
    BYTE d, e;
    BYTE h, l;

    MEMORY *memory;
} CPU;

CPU *CPU_Create();

/*
 * Bootstrap handles initialization of the CPU
 * void CPU_Init(CPU *c);
*/

void CPU_Destroy(CPU *c);


#endif // CPU_H