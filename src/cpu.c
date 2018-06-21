#include "cpu.h"
#include "decode.h"

#include <stdlib.h>
#include <string.h>


CPU *CPU_Create(){
    CPU *cpu = NULL;
    cpu = (CPU *) malloc(sizeof(CPU));
    if(cpu != NULL){
        cpu->memory = Mem_Create();
        if(cpu->memory == NULL){
            CPU_Destroy(cpu);
            cpu = NULL;
        }
    }
    return cpu;
}

// Initialization handled by bootstrap program

/*
void CPU_Init(CPU *c){
    if(c != NULL){
        c->sp = 0xFFFE;
    }
}
*/

void CPU_Destroy(CPU *c){
    if(c != NULL){
        Mem_Destroy(c->memory);
        free(c);
    }
}

void CPU_EmulateCycle(CPU *c){
    c->ir = Mem_ReadByte(c->memory, c->pc);
    Decode_Execute(c);
}

void CPU_SetFlag(CPU *c, BYTE flag, bool value){
    if(value == true){
        c->af.lo |= flag;
    }
    else{
        c->af.lo &= (~flag);
    }
}