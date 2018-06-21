#ifndef CPU_H
#define CPU_H

#include "common.h"
#include "memory.h"

static const BYTE Z_FLAG = 0x80; // Zero flag
static const BYTE N_FLAG = 0x40; // Subtract flag
static const BYTE H_FLAG = 0x20; // Half-Carry flag
static const BYTE C_FLAG = 0x10; // Carry flag

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
    bool halted;
    bool interrupt_enable;
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

void CPU_UpdateClockTimer(CPU *c, int cycles);

void CPU_SetInterrupt(CPU *c, bool e);

/*
static inline void CPU_SetFlag(CPU *c, BYTE flag) { c->af.lo |= flag; }
*/

static inline void CPU_SetFlag(CPU *c, BYTE flag, bool value){ c->af.lo = (value) ? (c->af.lo | flag) : (c->af.lo & (~flag)); }

static inline void CPU_ClearFlag(CPU *c, BYTE flag) { c->af.lo &= (~flag); }

static inline bool CPU_CheckFlag(CPU *c, BYTE cond) { return (c->af.lo & cond) == cond; }

#endif // CPU_H