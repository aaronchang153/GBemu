#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int BOOT_ROM_SIZE = 0x100;     // 256  B
static const int MEMORY_SIZE   = 0x10000;   //  64 KB

static const char *BOOT_ROM = "bootstrap.bin";


MEMORY *Mem_Create(){
    MEMORY *memory = NULL;
    memory = (MEMORY *) malloc(sizeof(MEMORY));
    if(memory != NULL){
        memory->boot = (BYTE *) malloc(BOOT_ROM_SIZE);
        memory->mem  = (BYTE *) malloc(MEMORY_SIZE);
        if(memory->boot == NULL || memory->mem == NULL){
            Mem_Destroy(memory);
            memory = NULL;
        }
        else{
            Mem_Init(memory);
        }
    }
    return memory;
}

void Mem_Init(MEMORY *mem){
    if(mem != NULL){
        mem->startup = 1;
        memset((void *) mem->mem, 0, MEMORY_SIZE);
        FILE *fp = fopen(BOOT_ROM, "rb");
        if(fp == NULL){
            printf("Failed to load boot rom.\n");
        }
        else{
            int i;
            for(i = 0; i < BOOT_ROM_SIZE; i++){
                fread((void *) &mem->boot[i], 1, 1, fp);
            }
        }
    }
}

void Mem_Destroy(MEMORY *mem){
    if(mem != NULL){
        if(mem->boot != NULL){
            free(mem->boot);
        }
        if(mem->mem != NULL){
            free(mem->mem);
        }
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