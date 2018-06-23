#include "cpu.h"
#include "decode.h"

#include <stdlib.h>
#include <string.h>


CPU *CPU_Create(){
    CPU *cpu = malloc(sizeof(CPU));
    return cpu;
}

void CPU_Startup(CPU *c){
    if(c != NULL){
        // Still needs to do a lot more
        c->pc = 0x0000;
        c->sp = 0xFFFE;
    }
}

void CPU_Destroy(CPU *c){
    if(c != NULL){
        free(c);
    }
}

void CPU_SetMemory(CPU *c, MEMORY *mem){
    c->memory = mem;
}

void CPU_Fetch(CPU *c){
    c->ir = Mem_ReadByte(c->memory, c->pc);
}

void CPU_DecodeExecute(CPU *c){
    Decode_Execute(c);
}

void CPU_EmulateCycle(CPU *c){
    c->ir = Mem_ReadByte(c->memory, c->pc);
    Decode_Execute(c);
}

void CPU_UpdateClockTimer(CPU *c, int cycles){
    c->memory->system_counter += cycles;
}

void CPU_SetInterrupt(CPU *c, bool e){

}