#include "interrupt.h"
#include "memory.h"

#include <stdlib.h>

static void ServiceInterrupt(INTERRUPT_HANDLER *i, BYTE type);


INTERRUPT_HANDLER *Interrupt_Create(){
    INTERRUPT_HANDLER *interrupt = malloc(sizeof(INTERRUPT_HANDLER));
    if(interrupt != NULL){
        interrupt->type = I_NONE;
        interrupt->cpu = NULL;
    }
    return interrupt;
}

void Interrupt_Destroy(INTERRUPT_HANDLER *interrupt){
    if(interrupt != NULL){
        free(interrupt);
    }
}

void Interrupt_SetCPU(INTERRUPT_HANDLER *interrupt, CPU *c){
    if(interrupt != NULL){
        interrupt->cpu =  c;
    }
}

void Interrupt_Handle(INTERRUPT_HANDLER *interrupt){
    if(interrupt->cpu->IME){
        BYTE requests = Mem_ReadByte(interrupt->cpu->memory, IF_ADDR);
        BYTE enable   = Mem_ReadByte(interrupt->cpu->memory, IE_ADDR);
        if(requests != 0 && enable != 0){
            if(TEST_FLAG(requests, IF_VBLANK) && TEST_FLAG(enable, IF_VBLANK)){
                ServiceInterrupt(interrupt, IF_VBLANK);
            }
            else if(TEST_FLAG(requests, IF_LCD_STAT) && TEST_FLAG(enable, IF_LCD_STAT)){
                ServiceInterrupt(interrupt, IF_LCD_STAT);
            }
            else if(TEST_FLAG(requests, IF_TIMER) && TEST_FLAG(enable, IF_TIMER)){
                ServiceInterrupt(interrupt, IF_TIMER);
            }
            else if(TEST_FLAG(requests, IF_JOYPAD) && TEST_FLAG(enable, IF_JOYPAD)){
                ServiceInterrupt(interrupt, IF_JOYPAD);
            }
        }
    }
}

static void ServiceInterrupt(INTERRUPT_HANDLER *i, BYTE type){
    // Push pc to stack
    Mem_WriteByte(i->cpu->memory, i->cpu->sp - 1, (i->cpu->pc & 0xFF00) >> 8);
    Mem_WriteByte(i->cpu->memory, i->cpu->sp - 2, (i->cpu->pc & 0x00FF));
    i->cpu->sp -= 2;
    i->cpu->cycles += 2 * 4;

    if(type == IF_VBLANK)
        i->cpu->pc = VBLANK_ROUTINE;
    else if(type == IF_LCD_STAT)
        i->cpu->pc = LCD_STAT_ROUTINE;
    else if(type == IF_TIMER)
        i->cpu->pc = TIMER_ROUTINE;
    else if(type == IF_JOYPAD)
        i->cpu->pc = JOYPAD_ROUTINE;
}