#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *BOOT_ROM = "bootstrap.bin";

static MEM_REGION Mem_GetRegion(MEMORY *mem, WORD addr);


MEMORY *Mem_Create(){
    MEMORY *memory = malloc(sizeof(MEMORY));
    return memory;
}

void Mem_Init(MEMORY *mem){
    if(mem != NULL){
        memset((void *) mem, 0, sizeof(MEMORY));
    }
}

void Mem_Destroy(MEMORY *mem){
    if(mem != NULL){
        free(mem);
    }
}

void Mem_WriteByte(MEMORY *mem, WORD addr, BYTE data){

}

BYTE Mem_ReadByte(MEMORY *mem, WORD addr){

}

WORD Mem_ReadWord(MEMORY *mem, WORD addr){

}

void Mem_IncByte(MEMORY *mem, WORD addr){

}

void Mem_DecByte(MEMORY *mem, WORD addr){

}

static MEM_REGION Mem_GetRegion(MEMORY *mem, WORD addr){
    if(mem->startup && addr < 0x100)
        return BOOT;
    else if(addr == 0xFFFF)
        return IE;
    else if(addr >= HRAM)
        return HRAM;
    else if(addr >= IO)
        return IO;
    else if(addr >= UNUSED)
        return UNUSED;
    else if(addr >= OAM)
        return OAM;
    else if(addr >= ECHO)
        return ECHO;
    else if(addr >= WRAMX)
        return WRAMX;
    else if(addr >= WRAM0)
        return WRAM0;
    else if(addr >= SRAM)
        return SRAM;
    else if(addr >= VRAM)
        return VRAM;
    else if(addr >= ROMX)
        return ROMX;
    else
        return ROM0;
}