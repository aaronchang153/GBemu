#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *BOOT_ROM = "bin\\bootstrap.bin";

static MEM_REGION Mem_GetRegion(MEMORY *mem, WORD addr);


MEMORY *Mem_Create(){
    MEMORY *memory = malloc(sizeof(MEMORY));
    if(memory != NULL){
        memset(memory, 0, sizeof(MEMORY));
        FILE *fp = fopen(BOOT_ROM, "rb");
        if(fp != NULL){
            for(int i = 0; i < 0x100; i++){
                fread(&memory->boot[i], 1, 1, fp);
            }
            fclose(fp);
        }
        memory->startup = true;
    }
    return memory;
}

void Mem_LoadGame(MEMORY *mem, char *filename){
    if(mem != NULL && filename != NULL){
        Mem_UnloadGame(mem);
        FILE *fp = fopen(filename, "rb");
        if(fp != NULL){
            fseek(fp, 0L, SEEK_END);
            size_t size = ftell(fp);
            rewind(fp);
            mem->game_rom = malloc(size);
            if(mem->game_rom != NULL){
                for(int i = 0; i < size; i++){
                    fread(mem->game_rom + i, 1, 1, fp);
                }
            }
            fclose(fp);
        }
    }
}

void Mem_UnloadGame(MEMORY *mem){
    if(mem != NULL && mem->game_rom != NULL){
        free(mem->game_rom);
        mem->game_rom = NULL;
    }
}

void Mem_Startup(MEMORY *mem){
    if(mem != NULL && mem->game_rom != NULL){
        mem->rom0 = mem->game_rom;
    }
}

void Mem_Destroy(MEMORY *mem){
    if(mem != NULL){
        if(mem->game_rom != NULL)
            free(mem->game_rom);
        free(mem);
    }
}

void Mem_WriteByte(MEMORY *mem, WORD addr, BYTE data){
    switch(Mem_GetRegion(mem, addr)){
        case SRAM:
            mem->sram[addr - 0xA000] = data;
            break;
        case VRAM:
            mem->vram[addr - 0x8000] = data;
            break;
        case WRAM0:
        case WRAMX:
        case OAM:
        case HRAM:
        case IE:
        case IO:
            mem->mem[addr - 0xC000] = data;
            break;
        case ECHO:
            mem->mem[addr - 0xE000] = data;
            break;
        default:
            printf("Unable to write to address: 0x%04x\n", addr);
    };
}

BYTE Mem_ReadByte(MEMORY *mem, WORD addr){
    switch(Mem_GetRegion(mem, addr)){
        case BOOT:
            if(addr == 0xFF)
                mem->startup = false;
            return mem->boot[addr];
        case ROMX:
            return mem->romx[addr - 0x4000];
        case SRAM:
            return mem->sram[addr - 0xA000];
        case VRAM:
            return mem->vram[addr - 0x8000];
        case WRAM0:
        case WRAMX:
        case OAM:
        case HRAM:
        case IE:
            return mem->mem[addr - 0xC000];
        case ECHO:
            // May need to do more than just this
            return mem->mem[addr - 0xE000];
        case UNUSED:
            return 0x00;
        case IO:
            // Temporary
            return 0x01;
        default:
            printf("Invalid read address: %x\n", addr);
            return 0;
    };
}

WORD Mem_ReadWord(MEMORY *mem, WORD addr){
    return (Mem_ReadByte(mem, addr + 1) << 8) | Mem_ReadByte(mem, addr);
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