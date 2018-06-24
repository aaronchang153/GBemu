#include "interrupt.h"
#include "memory.h"


static void ServiceInterrupt(CPU *c, WORD service_routine);

void Interrupt_Handle(CPU *c){
    if(c->IME){
        BYTE requests = Mem_ReadByte(c->memory, IF_ADDR);
        BYTE enable   = Mem_ReadByte(c->memory, IE_ADDR);
        if(requests != 0 && enable != 0){
            // If there's an interrupt to handle, handle it and disable further interrupts
            if(TEST_FLAG(requests, IF_VBLANK) && TEST_FLAG(enable, IF_VBLANK)){
                ServiceInterrupt(c, VBLANK_ROUTINE);
            }
            else if(TEST_FLAG(requests, IF_LCD_STAT) && TEST_FLAG(enable, IF_LCD_STAT)){
                ServiceInterrupt(c, LCD_STAT_ROUTINE);
            }
            else if(TEST_FLAG(requests, IF_TIMER) && TEST_FLAG(enable, IF_TIMER)){
                ServiceInterrupt(c, TIMER_ROUTINE);
            }
            else if(TEST_FLAG(requests, IF_JOYPAD) && TEST_FLAG(enable, IF_JOYPAD)){
                ServiceInterrupt(c, JOYPAD_ROUTINE);
            }
            c->IME = false;
        }
    }
}

static void ServiceInterrupt(CPU *c, WORD service_routine){
    // Push pc to stack
    Mem_WriteByte(c->memory, c->sp - 1, (c->pc & 0xFF00) >> 8);
    Mem_WriteByte(c->memory, c->sp - 2, (c->pc & 0x00FF));
    c->sp -= 2;
    c->pc = service_routine;
    c->cycles += 3 * 4;
}