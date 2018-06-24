#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "common.h"
#include "cpu.h"

static const WORD VBLANK_ROUTINE    = 0x40;
static const WORD LCD_STAT_ROUTINE  = 0x48;
static const WORD TIMER_ROUTINE     = 0x50;
static const WORD JOYPAD_ROUTINE    = 0x60;


typedef enum{
    I_NONE,
    I_VBLANK,
    I_LCD_STAT,
    I_TIMER,
    I_SERIAL,
    I_JOYPAD
} INTERRUPT_TYPE;

typedef struct{
    INTERRUPT_TYPE type;
    CPU *cpu;
} INTERRUPT_HANDLER;

INTERRUPT_HANDLER *Interrupt_Create();

void Interrupt_Destroy(INTERRUPT_HANDLER *interrupt);

void Interrupt_SetCPU(INTERRUPT_HANDLER *interrupt, CPU *c);

void Interrupt_Handle(INTERRUPT_HANDLER *interrupt);


#endif // INTERRUPT_H