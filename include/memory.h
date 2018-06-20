#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"


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

void Mem_IncByte(MEMORY *mem, WORD addr);

void Mem_DecByte(MEMORY *mem, WORD addr);

#endif // MEMORY_H