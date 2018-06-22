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

void CPU_Init(CPU *c){
    if(c != NULL){
        // Still needs to do a lot more
        c->sp = 0xFFFE;
    }
}

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

void CPU_UpdateClockTimer(CPU *c, int cycles){

}

void CPU_SetInterrupt(CPU *c, bool e){

}