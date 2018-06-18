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
    WORD pc;
    WORD sp;
    REGISTER af;
    REGISTER bc;
    REGISTER de;
    REGISTER hl;

    MEMORY *memory;
};

CPU *CPU_Create();

/*
 * Bootstrap handles initialization of the CPU
 * void CPU_Init(CPU *c);
*/

void CPU_Destroy(CPU *c);


#endif // CPU_H