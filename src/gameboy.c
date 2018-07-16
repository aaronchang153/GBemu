#include "gameboy.h"

#include <stdlib.h>

static const unsigned int CYCLES_PER_UPDATE = CLK_F / UPDATES_PER_SEC;

GAMEBOY *GB_Create(){
    GAMEBOY *gb = malloc(sizeof(GAMEBOY));
    if(gb != NULL){
        gb->cpu = CPU_Create();
        gb->memory = Mem_Create();
        gb->timer = Timer_Create();
        gb->graphics = Graphics_Create();
        gb->display = Display_Create();
        gb->joypad = Joypad_Create();
        gb->apu = APU_Create();
        if(gb->cpu == NULL || gb->memory == NULL || gb->timer == NULL ||
           gb->graphics == NULL || gb->display == NULL || gb->joypad == NULL || gb->apu == NULL)
        {
            GB_Destroy(gb);
            gb = NULL;
        }
        else{
            // Connect all the components that need to be connected
            CPU_SetMemory(gb->cpu, gb->memory);
            Timer_SetMemory(gb->timer, gb->memory);
            Graphics_SetMemory(gb->graphics, gb->memory);
            Graphics_SetDisplay(gb->graphics, gb->display);
            Mem_SetJoypad(gb->memory, gb->joypad);
            APU_SetMemory(gb->apu, gb->memory);
        }
    }
    return gb;
}

void GB_Destroy(GAMEBOY *gb){
    if(gb != NULL){
        CPU_Destroy(gb->cpu);
        Mem_Destroy(gb->memory);
        Timer_Destroy(gb->timer);
        Graphics_Destroy(gb->graphics);
        Display_Destroy(gb->display);
        Joypad_Destroy(gb->joypad);
        APU_Destroy(gb->apu);
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
        if(!gb->cpu->stop){
            if(!gb->cpu->halt){
                CPU_EmulateCycle(gb->cpu);
                cycles = CPU_GetCycles(gb->cpu);
                total_cycles += cycles;
                APU_Update(gb->apu, (int) cycles);
            }
            else{
                cycles = 4;
                total_cycles += 4;
            } // endif halt
            Timer_Update(gb->timer, cycles);
            Graphics_Update(gb->graphics, cycles);
        }
        else{
            cycles = 4;
            total_cycles += 4;
        } // endif stop
        Interrupt_Handle(gb->cpu);
    }
    Graphics_RenderScreen(gb->graphics);
}