#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


MEMORY *Mem_Create(){
    MEMORY *memory = malloc(sizeof(MEMORY));
    if(memory != NULL){
        memset(memory, 0, sizeof(MEMORY));
        if((memory->cartridge = Cartridge_Create()) == NULL){
            Mem_Destroy(memory);
            memory = NULL;
        }
    }
    return memory;
}

void Mem_LoadGame(MEMORY *mem, char *filename){
    Cartridge_LoadGame(mem->cartridge, filename);
}

void Mem_UnloadGame(MEMORY *mem){
    Cartridge_UnloadGame(mem->cartridge);
}

void Mem_Startup(MEMORY *mem){
    if(mem != NULL){
        Cartridge_Init(mem->cartridge);

        Mem_ForceWrite(mem, 0xFF05, 0x00); // TIMA
        Mem_ForceWrite(mem, 0xFF06, 0x00); // TMA
        Mem_ForceWrite(mem, 0xFF07, 0x00); // TAC
        Mem_ForceWrite(mem, 0xFF10, 0x80); // NR10
        Mem_ForceWrite(mem, 0xFF11, 0xBF); // NR11
        Mem_ForceWrite(mem, 0xFF12, 0xF3); // NR12
        Mem_ForceWrite(mem, 0xFF14, 0xBF); // NR14
        Mem_ForceWrite(mem, 0xFF16, 0x3F); // NR21
        Mem_ForceWrite(mem, 0xFF17, 0x00); // NR22
        Mem_ForceWrite(mem, 0xFF19, 0xBF); // NR24
        Mem_ForceWrite(mem, 0xFF1A, 0x7F); // NR30
        Mem_ForceWrite(mem, 0xFF1B, 0xFF); // NR31
        Mem_ForceWrite(mem, 0xFF1C, 0x9F); // NR32
        Mem_ForceWrite(mem, 0xFF1E, 0xBF); // NR33
        Mem_ForceWrite(mem, 0xFF20, 0xFF); // NR41
        Mem_ForceWrite(mem, 0xFF21, 0x00); // NR42
        Mem_ForceWrite(mem, 0xFF22, 0x00); // NR43
        Mem_ForceWrite(mem, 0xFF23, 0xBF); // NR44
        Mem_ForceWrite(mem, 0xFF24, 0x77); // NR50
        Mem_ForceWrite(mem, 0xFF25, 0xF3); // NR51
        Mem_ForceWrite(mem, 0xFF26, 0xF1), // NR52
        Mem_ForceWrite(mem, 0xFF40, 0x91); // LCDC
        Mem_ForceWrite(mem, 0xFF42, 0x00); // SCY
        Mem_ForceWrite(mem, 0xFF43, 0x00); // SCX
        Mem_ForceWrite(mem, 0xFF45, 0x00); // LYC
        Mem_ForceWrite(mem, 0xFF47, 0xFC); // BGP
        Mem_ForceWrite(mem, 0xFF48, 0xFF); // OBP0
        Mem_ForceWrite(mem, 0xFF49, 0xFF); // OBP1
        Mem_ForceWrite(mem, 0xFF4A, 0x00); // WY
        Mem_ForceWrite(mem, 0xFF4B, 0x00); // WX
        Mem_ForceWrite(mem, 0xFFFF, 0x00); // IE
    }
}

void Mem_Destroy(MEMORY *mem){
    if(mem != NULL){
        Cartridge_Destroy(mem->cartridge);
        free(mem);
    }
}

void Mem_SetJoypad(MEMORY *mem, JOYPAD *j){
    mem->joypad = j;
}

void Mem_WriteByte(MEMORY *mem, WORD addr, BYTE data){
    switch(Mem_GetRegion(mem, addr)){
        case ROM0:
        case ROMX:
            Cartridge_SwitchBank(mem->cartridge, addr, data);
            break;
        case SRAM:
            Cartridge_WriteRAM(mem->cartridge, addr, data);
            break;
        case VRAM:
            mem->vram[addr - 0x8000] = data;
            break;
        case WRAM0:
        case WRAMX:
        case OAM:
        case HRAM:
        case IE:
            mem->mem[addr - 0xC000] = data;
            break;
        case IO:
            switch(addr){
                case P1_ADDR:
                    // Don't write the lower 4 bits
                    mem->mem[addr - 0xC000] = data & 0xF0;
                    break;
                case DIV_ADDR:
                case LY_ADDR:
                    mem->mem[addr - 0xC000] = 0;
                    break;
                case TAC_ADDR: // Write first 3 bits only
                    mem->mem[addr - 0xC000] = data & 0x07;
                    break;
                case DMA_ADDR:
                    Mem_DMATransfer(mem, data);
                    break;
                default:
                    mem->mem[addr - 0xC000] = data;
            };
            break;
        case ECHO:
            mem->mem[addr - 0xE000] = data;
            break;
        default:
            // Tetris still tries to write here so I guess it's not a big deal
            // printf("Unable to write to address: 0x%04x\n", addr);
            break;
    };
}

void Mem_WriteWord(MEMORY *mem, WORD addr, WORD data){
    Mem_WriteByte(mem, addr, data & 0x00FF);
    Mem_WriteByte(mem, addr + 1, (data & 0xFF00) >> 8);
}

void Mem_DMATransfer(MEMORY *mem, BYTE data){
    WORD addr = data << 8;
    for(int i = 0; i < 0xA0; i++){
        Mem_WriteByte(mem, addr, Mem_ReadByte(mem, addr + i));
    }
}

BYTE Mem_ReadByte(MEMORY *mem, WORD addr){
    MEM_REGION region = Mem_GetRegion(mem, addr);
    switch(region){
        case ROM0:
        case ROMX:
            return Cartridge_ReadROM(mem->cartridge, addr);
        case SRAM:
            return Cartridge_ReadRAM(mem->cartridge, addr);
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
            if(addr == P1_ADDR){
                return Joypad_GetState(mem->joypad, mem->mem[addr - 0xC000]);
            }
            else if(addr == TAC_ADDR){
                return mem->mem[addr - 0xC000] & 0x07;
            }
            else if(addr == IF_ADDR){
                return mem->mem[addr - 0xC000] | 0xE0;
            }
            else{
                return mem->mem[addr - 0xC000];
                //return 0x01;
            }
        default:
            printf("Invalid read address: 0x%x\n", addr);
            return 0;
    };
}

WORD Mem_ReadWord(MEMORY *mem, WORD addr){
    return (Mem_ReadByte(mem, addr + 1) << 8) | Mem_ReadByte(mem, addr);
}

void Mem_RequestInterrupt(MEMORY *mem, BYTE interrupt){
    mem->mem[0xFF0F - 0xC000] |= interrupt;
}

void Mem_EnableInterrupt(MEMORY *mem, BYTE interrupt){
    mem->mem[0xFFFF - 0xC000] |= interrupt; 
}

void Mem_DisableInterrupt(MEMORY *mem, BYTE interrupt){
    mem->mem[0xFFFF - 0xC000] &= ~(interrupt);
}

MEM_REGION Mem_GetRegion(MEMORY *mem, WORD addr){
    if(addr == 0xFFFF)
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

void Mem_ForceWrite(MEMORY *mem, WORD addr, BYTE data){
    switch(Mem_GetRegion(mem, addr)){
        case VRAM:
            mem->vram[addr - 0x8000] = data;
            break;
        case WRAM0:
        case WRAMX:
        case ECHO:
        case OAM:
        case HRAM:
        case IE:
        case IO:
            mem->mem[addr - 0xC000] = data;
            break;
        default:
            printf("Address either cannot be reached or is part of the game cartridge: 0x%04x\n", addr);
    };
}