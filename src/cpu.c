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
        c->pc = 0x0100;
        c->sp = 0xFFFE;
        c->af.reg = 0x01B0;
        c->bc.reg = 0x0013;
        c->de.reg = 0x00D8;
        c->hl.reg = 0x014D;
        c->IME = false;
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

unsigned int CPU_GetCycles(CPU *c){
    return c->cycles;
}

void CPU_SetCycles(CPU *c, int cycles){
    c->cycles = cycles;
}

void CPU_SetInterrupt(CPU *c, bool e){

}