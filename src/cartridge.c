#include "cartridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void SwitchRAMEnable(CARTRIDGE *cart, WORD addr, BYTE data);
static void SwitchROMBankLo(CARTRIDGE *cart, BYTE data);
static void SwitchROMBankHi(CARTRIDGE *cart, BYTE data);
static void SwitchRAMBank(CARTRIDGE *cart, BYTE data);
static void SwitchROMRAMMode(CARTRIDGE *cart, BYTE data);


CARTRIDGE *Cartridge_Create(){
    CARTRIDGE *cartridge = malloc(sizeof(CARTRIDGE));
    if(cartridge != NULL){
        memset(cartridge, 0, sizeof(CARTRIDGE));
        cartridge->type = NONE;
    }
    return cartridge;
}

void Cartridge_Destroy(CARTRIDGE *cart){
    if(cart != NULL){
        if(cart->game_rom != NULL)
            free(cart->game_rom);
        free(cart);
    }
}

void Cartridge_LoadGame(CARTRIDGE *cart, char *filename){
    if(cart != NULL && filename != NULL){
        Cartridge_UnloadGame(cart);
        FILE *fp = fopen(filename, "rb");
        if(fp != NULL){
            fseek(fp, 0L, SEEK_END);
            size_t size = ftell(fp);
            rewind(fp);
            cart->game_rom = malloc(size);
            if(cart->game_rom != NULL){
                for(int i = 0; i < size; i++){
                    fread(cart->game_rom + i, 1, 1, fp);
                }
            }
            fclose(fp);
        }
    }
}

void Cartridge_UnloadGame(CARTRIDGE *cart){
    if(cart != NULL && cart->game_rom != NULL){
        free(cart->game_rom);
    }
}

void Cartridge_Init(CARTRIDGE *cart){
    if(cart != NULL && cart->game_rom != NULL){
        switch(cart->game_rom[0x0147]){
            case 0:
                cart->type = NONE;
                break;
            case 1:
            case 2:
            case 3:
                cart->type = MBC1;
                break;
            case 5:
            case 6:
                cart->type = MBC2;
                break;
        };
        // Switchable ROM bank initialized to the next bank after ROM bank 0
        cart->current_rom_bank = 1;
        // SRAM bank initialized to RAM bank 0 (if it exists)
        cart->current_ram_bank = 0;
    }
}

void Cartridge_SwitchBank(CARTRIDGE *cart, WORD addr, BYTE data){
    if(addr < 0x2000){
        if(cart->type == MBC1 || cart->type == MBC2){
            SwitchRAMEnable(cart, addr, data);
        }
    }
    else if((addr >= 0x2000) && (addr < 0x4000)){
        if(cart->type == MBC1 || cart->type == MBC2){
            SwitchROMBankLo(cart, data);
        }
    }
    else if((addr >= 0x4000) && (addr < 0x6000)){
        if(cart->type == MBC1){
            if(cart->rom_banking){
                SwitchROMBankHi(cart, data);
            }
            else{
                SwitchRAMBank(cart, data);
            }
        }
    }
    else if((addr >= 0x6000) && (addr < 0x8000)){
        if(cart->type == MBC1){
            SwitchROMRAMMode(cart, data);
        }
    }
}

BYTE Cartridge_ReadROM(CARTRIDGE *cart, WORD addr){
    if(addr < 0x4000)
        return cart->game_rom[addr];
    else if((addr >= 0x4000) && (addr < 0x8000))
        return cart->game_rom[(addr - 0x4000) + (cart->current_ram_bank * ROM_BANK_SIZE)];
    else
        return 0;
}

BYTE Cartridge_ReadRAM(CARTRIDGE *cart, WORD addr){
    if(cart->ram_enabled && (addr >= 0xA000) && (addr < 0xC000))
        return cart->ram[(addr - 0xA000) + (cart->current_ram_bank * RAM_BANK_SIZE)];
    else
        return 0;
}

void Cartridge_WriteRAM(CARTRIDGE *cart, WORD addr, BYTE data){
    if(cart->ram_enabled && (addr >= 0xA000) && (addr < 0xC000))
        cart->ram[(addr - 0xA000) + (cart->current_ram_bank * RAM_BANK_SIZE)] = data;
}

static void SwitchRAMEnable(CARTRIDGE *cart, WORD addr, BYTE data){
    if(cart->type != MBC2 || !TEST_BIT(addr, 4)){
        cart->ram_enabled = ((data & 0x0F) == 0x0A) ? true : false;
    }
}

static void SwitchROMBankLo(CARTRIDGE *cart, BYTE data){
    if(cart->type == MBC2){
        cart->current_rom_bank = data & 0x0F;
    }
    else{
        // Clear lower 5 bits
        cart->current_rom_bank &= ~(0x1F);
        // Set lower 5 bits
        cart->current_rom_bank |= (data & 0x1F);
    }

    if(cart->current_rom_bank == 0)
        cart->current_rom_bank++;
}

static void SwitchROMBankHi(CARTRIDGE *cart, BYTE data){
    // Clear upper 3 bits
    cart->current_rom_bank &= 0x1F;
    // Set upper 3 bits
    cart->current_rom_bank |= (data & ~(0x1F));
    if(cart->current_rom_bank == 0)
        cart->current_rom_bank++;
}

static void SwitchRAMBank(CARTRIDGE *cart, BYTE data){
    // Only gets called for MBC1
    cart->current_ram_bank = data & 0x03;
}

static void SwitchROMRAMMode(CARTRIDGE *cart, BYTE data){
    cart->rom_banking = ((data & 0x01) == 0) ? true : false;
    if(cart->rom_banking)
        cart->current_ram_bank = 0;
}