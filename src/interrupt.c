#include "interrupt.h"
#include "memory.h"

static void ServiceInterrupt(CPU *c, BYTE type);


void Interrupt_Handle(CPU *c){
    if(c->IME){
        BYTE requests = Mem_ReadByte(c->memory, IF_ADDR);
        BYTE enable   = Mem_ReadByte(c->memory, IE_ADDR);
        if(requests != 0 && enable != 0){
            // If there's an interrupt to handle, handle it and disable further interrupts
            if(TEST_FLAG(requests, IF_VBLANK) && TEST_FLAG(enable, IF_VBLANK)){
                ServiceInterrupt(c, IF_VBLANK);
            }
            else if(TEST_FLAG(requests, IF_LCD_STAT) && TEST_FLAG(enable, IF_LCD_STAT)){
                ServiceInterrupt(c, IF_LCD_STAT);
            }
            else if(TEST_FLAG(requests, IF_TIMER) && TEST_FLAG(enable, IF_TIMER)){
                ServiceInterrupt(c, IF_TIMER);
            }
            else if(TEST_FLAG(requests, IF_JOYPAD) && TEST_FLAG(enable, IF_JOYPAD)){
                ServiceInterrupt(c, IF_JOYPAD);
            }
            c->IME = false;
        }
    }
}

static void ServiceInterrupt(CPU *c, BYTE type){
    // Push pc to stack
    Mem_WriteByte(c->memory, c->sp - 1, (c->pc & 0xFF00) >> 8);
    Mem_WriteByte(c->memory, c->sp - 2, (c->pc & 0x00FF));
    c->sp -= 2;

    if(type == IF_VBLANK)
        c->pc = VBLANK_ROUTINE;
    else if(type == IF_LCD_STAT)
        c->pc = LCD_STAT_ROUTINE;
    else if(type == IF_TIMER)
        c->pc = TIMER_ROUTINE;
    else if(type == IF_JOYPAD)
        c->pc = JOYPAD_ROUTINE;

    c->cycles += 3 * 4;
}