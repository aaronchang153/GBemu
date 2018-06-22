#include "cpu.h"
#include "memory.h"

#include <stdio.h>

#ifdef DEBUG
#include "cpudebug.h"
#endif // DEBUG


int main(){
    CPU *cpu = CPU_Create();
    MEMORY *memory = Mem_Create();
    CPU_SetMemory(cpu, memory);
    Mem_Startup(memory);
    CPU_Startup(cpu);
#ifdef DEBUG
    Start_Debugger(cpu);
#endif // DEBUG
    CPU_Destroy(cpu);
    Mem_Destroy(memory);
    return 0;
}