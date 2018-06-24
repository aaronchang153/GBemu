#include "gameboy.h"

#include <stdlib.h>

static const unsigned int CYCLES_PER_UPDATE = CLK_F / UPDATES_PER_SEC;

GAMEBOY *GB_Create(){
    GAMEBOY *gb = malloc(sizeof(GAMEBOY));
    if(gb != NULL){
        gb->cpu = CPU_Create();
        gb->memory = Mem_Create();
        gb->timer = Timer_Create();
        if(gb->cpu == NULL || gb->memory == NULL || gb->timer == NULL){
            GB_Destroy(gb);
            gb = NULL;
        }
        else{
            CPU_SetMemory(gb->cpu, gb->memory);
            Timer_SetMemory(gb->timer, gb->memory);
        }
    }
    return gb;
}

void GB_Destroy(GAMEBOY *gb){
    if(gb != NULL){
        CPU_Destroy(gb->cpu);
        Mem_Destroy(gb->memory);
        Timer_Destroy(gb->timer);
        free(gb);
    }
}

void GB_LoadGame(GAMEBOY *gb, char *filename){
    if(gb != NULL && gb->memory != NULL){
        Mem_LoadGame(gb->memory, filename);
    }
}

void GB_Startup(GAMEBOY *gb){
    if(gb != NULL){
        CPU_Startup(gb->cpu);
        Mem_Startup(gb->memory);
    }
}

void GB_Update(GAMEBOY *gb){
    unsigned int total_cycles = 0;
    unsigned int cycles;

    while(total_cycles < CYCLES_PER_UPDATE){
        CPU_EmulateCycle(gb->cpu);
        cycles = CPU_GetCycles(gb->cpu);
        total_cycles += cycles;
        Timer_Update(gb->timer, cycles);
        /****** Update Graphics ******/
        /***** Handle Interrupts *****/
    }
    /***** Render Screen *****/
}