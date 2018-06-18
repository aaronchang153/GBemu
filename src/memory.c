#include "memory.h"

const char *BOOT_ROM = "bootstrap.bin";


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