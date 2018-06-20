#ifndef CPU_H
#define CPU_H

#include "common.h"
#include "memory.h"

typedef struct cpu CPU;
typedef union reg REGISTER;

union reg{
    struct{
        BYTE lo;
        BYTE hi;
    };
    WORD reg;
};

struct cpu{
    BYTE ir;
    WORD pc;
    WORD sp;
    REGISTER af;
    REGISTER bc;
    REGISTER de;
    REGISTER hl;
    char halted;
    char interrupt_enable;
    unsigned int clk_timer;

    MEMORY *memory;
};

CPU *CPU_Create();

/*
 * Bootstrap handles initialization of the CPU
 * void CPU_Init(CPU *c);
*/

void CPU_Destroy(CPU *c);

// Emulate a single instruction cycle
void CPU_EmulateCycle(CPU *c);

void CPU_DecodeExecute(CPU *c);

static inline void CPU_SetFlag(CPU *c, BYTE flag) { c->af.lo |= flag; }

static inline void CPU_ClearFlag(CPU *c, BYTE flag) { c->af.lo &= (~flag); }

static inline char CPU_CheckFlag(CPU *c, BYTE cond) { return (c->af.lo & cond) == cond; }

#endif // CPU_H