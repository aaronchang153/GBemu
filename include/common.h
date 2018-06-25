#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#define CLK_F           4194304 // Hz
#define CYCLE_F         1048576 // Hz
#define UPDATES_PER_SEC      60

// Commonly used types
typedef unsigned char BYTE;
typedef char SIGNED_BYTE;
typedef unsigned short WORD;
typedef short SIGNED_WORD;

// Interrupt Flags (Descending Priority)
static const BYTE IF_VBLANK   = 0x01;
static const BYTE IF_LCD_STAT = 0x02;
static const BYTE IF_TIMER    = 0x04;
static const BYTE IF_SERIAL   = 0x08;
static const BYTE IF_JOYPAD   = 0x10;

static inline bool TEST_BIT(BYTE reg, int bit) { return (reg & (0x01 << bit)) != 0x00; }

static inline bool TEST_FLAG(BYTE reg, BYTE flag) { return (reg & flag) == flag; }

#endif // COMMON_H