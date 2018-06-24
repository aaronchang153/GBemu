#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "common.h"
#include "cpu.h"

static const WORD VBLANK_ROUTINE    = 0x40;
static const WORD LCD_STAT_ROUTINE  = 0x48;
static const WORD TIMER_ROUTINE     = 0x50;
static const WORD JOYPAD_ROUTINE    = 0x60;


void Interrupt_Handle(CPU *c);


#endif // INTERRUPT_H