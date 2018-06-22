#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

/*
 * DMG Memory Map:
 * > Taken from giibiiadvanced docs
 * > https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf
 * 
 * 0000-3FFF: [ROM0] Non-switchable ROM bank
 *     > 0000-0100: [BOOT] Boot ROM (on startup)
 * 4000-7FFF: [ROMX] Switchable ROM bank
 * 8000-9FFF: [VRAM] Video RAM
 * A000-BFFF: [SRAM] External RAM in cartridge
 * C000-CFFF: [WRAM0] Work RAM
 * D000-DFFF: [WRAMX] Work RAM (switchable in GBC)
 * E000-FDFF: [ECHO] Complicated, but usually just echos WRAM
 * FE00-FE9F: [OAM] Sprite information table
 * FEA0-FEFF: <UNUSED>
 * FF00-FF7F: [IO] I/O Registers
 * FF80-FFFE: [HRAM] Internal CPU RAM
 *      FFFF: [IE] Interrupt enable flags
 * 
 */

typedef enum{
    BOOT    = 0x0000,
    ROM0    = 0x0000,
    ROMX    = 0x4000,
    VRAM    = 0x8000,
    SRAM    = 0xA000,
    WRAM0   = 0xC000,
    WRAMX   = 0xD000,
    ECHO    = 0xE000,
    OAM     = 0xFE00,
    UNUSED  = 0xFEA0,
    IO      = 0xFF00,
    HRAM    = 0xFF80,
    IE      = 0xFFFF
} MEM_REGION;


typedef struct{
    bool startup;
    BYTE boot[0x100];
    BYTE memory[0x10000];
} MEMORY;

MEMORY *Mem_Create();

void Mem_Init(MEMORY *mem);

void Mem_Destroy(MEMORY *mem);

void Mem_WriteByte(MEMORY *mem, WORD addr, BYTE data);

//void Mem_WriteWord(MEMORY *mem, WORD addr, WORD data);

BYTE Mem_ReadByte(MEMORY *mem, WORD addr);

WORD Mem_ReadWord(MEMORY *mem, WORD addr);

void Mem_IncByte(MEMORY *mem, WORD addr);

void Mem_DecByte(MEMORY *mem, WORD addr);

#endif // MEMORY_H