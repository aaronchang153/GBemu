#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#define CLK_F   4194304 // Hz
#define CYCLE_F 1048576 // Hz

// Important addresses in memory
#define DIV_ADDR            0xFF04
#define TIMA_COUNTER_ADDR   0xFF05
#define TIMA_MODULO_ADDR    0xFF06
#define TAC_ADDR            0xFF07
#define IF_ADDR             0xFF0F

// Commonly used types
typedef unsigned char BYTE;
typedef char SIGNED_BYTE;
typedef unsigned short WORD;
typedef short SIGNED_WORD;

// Interrupt Enable Flags
static const BYTE IEF_JOYPAD   = 0x10;
static const BYTE IEF_SERIAL   = 0x08;
static const BYTE IEF_TIMER    = 0x04;
static const BYTE IEF_LCD_STAT = 0x02;
static const BYTE IEF_VBLANK   = 0x01;

#endif // COMMON_H