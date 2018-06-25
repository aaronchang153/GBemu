#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

/*
 * DMG Memory Map:
 * > Taken from giibiiadvanced docs
 * > https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf
 * 
 * 0000-3FFF: [ROM0]    Non-switchable ROM bank
 *      > 0000-00FF:    [BOOT] Boot ROM (on startup)
 * 4000-7FFF: [ROMX]    Switchable ROM bank
 * 8000-9FFF: [VRAM]    Video RAM
 * A000-BFFF: [SRAM]    External RAM in cartridge
 * C000-CFFF: [WRAM0]   Work RAM
 * D000-DFFF: [WRAMX]   Work RAM (switchable in GBC)
 * E000-FDFF: [ECHO]    Complicated, but usually just echos WRAM
 * FE00-FE9F: [OAM]     Sprite information table
 * FEA0-FEFF:           <UNUSED>
 * FF00-FF7F: [IO]      I/O Registers
 * FF80-FFFE: [HRAM]    Internal CPU RAM
 *      FFFF: [IE]      Interrupt enable flags
 * 
 */

// Important addresses in memory
#define DIV_ADDR    0xFF04
#define TIMA_ADDR   0xFF05
#define TMA_ADDR    0xFF06
#define TAC_ADDR    0xFF07
#define IF_ADDR     0xFF0F
#define IE_ADDR     0xFFFF

#define DMA_ADDR    0xFF46

#define LY_ADDR     0xFF44 // Current LCD scanline


typedef enum{
    //BOOT    = 0x0000,
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

// 64 KB Byte-Addressable Memory
typedef struct{
    //bool startup;
    //BYTE boot[0x100];   // 256  B: Boot ROM
    BYTE *game_rom;     //         Entire game cartridge
    BYTE *rom0;         //  16 KB: Unswitchable ROM bank 0
    BYTE *romx;         //  16 KB: Current (switable) ROM bank
    BYTE vram[0x2000];  //   8 KB: VRAM
    BYTE *sram;         //   8 KB: Current external RAM
    BYTE mem[0x4000];   //  16 KB: Remaining memory
} MEMORY;

MEMORY *Mem_Create();

void Mem_LoadGame(MEMORY *mem, char *filename);

void Mem_UnloadGame(MEMORY *mem);

void Mem_Startup(MEMORY *mem);

void Mem_Destroy(MEMORY *mem);

void Mem_WriteByte(MEMORY *mem, WORD addr, BYTE data);

void Mem_WriteWord(MEMORY *mem, WORD addr, WORD data);

void Mem_DMATransfer(MEMORY *mem, BYTE data);

BYTE Mem_ReadByte(MEMORY *mem, WORD addr);

WORD Mem_ReadWord(MEMORY *mem, WORD addr);

void Mem_RequestInterrupt(MEMORY *mem, BYTE interrupt);

void Mem_EnableInterrupt(MEMORY *mem, BYTE interrupt);

void Mem_DisableInterrupt(MEMORY *mem, BYTE interrupt);

MEM_REGION Mem_GetRegion(MEMORY *mem, WORD addr);

void Mem_ForceWrite(MEMORY *mem, WORD addr, BYTE data);

#endif // MEMORY_H