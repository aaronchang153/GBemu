#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOOT_ROM_SIZE 0x100     // 256  B
#define MEMORY_SIZE   0x10000   //  64 KB


typedef struct{
    char startup;
    BYTE *boot;
    BYTE *mem;
} MEMORY;

MEMORY *Mem_Create();

void Mem_Init(MEMORY *mem);

void Mem_Destroy(MEMORY *mem);

void Mem_WriteByte(MEMORY *mem, WORD addr, BYTE data);

void Mem_WriteWord(MEMORY *mem, WORD addr, WORD data);

BYTE Mem_ReadByte(MEMORY *mem, WORD addr);

WORD Mem_ReadWord(MEMORY *mem, WORD addr);


#endif // MEMORY_H