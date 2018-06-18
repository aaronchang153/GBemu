#include "cpu.h"

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