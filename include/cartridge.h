#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "common.h"

#define ROM_BANK_SIZE 0x4000
#define RAM_BANK_SIZE 0x2000

#define MAX_RAM_BANKS 4

typedef enum{
    NONE,
    MBC1,
    MBC2
} MBC_TYPE;

typedef struct{
    MBC_TYPE type;
    BYTE *game_rom;
    BYTE ram[RAM_BANK_SIZE * MAX_RAM_BANKS];
    bool ram_enabled;
    bool rom_banking; // If true, cartridge is in ROM banking mode
                   // otherwise, it's in RAM banking mode
    BYTE current_rom_bank;
    BYTE current_ram_bank;
} CARTRIDGE;


CARTRIDGE *Cartridge_Create();

void Cartridge_Destroy(CARTRIDGE *cart);

void Cartridge_LoadGame(CARTRIDGE *cart, char *filename);

void Cartridge_UnloadGame(CARTRIDGE *cart);

void Cartridge_Init(CARTRIDGE *cart);

void Cartridge_SwitchBank(CARTRIDGE *cart, WORD addr, BYTE data);

BYTE Cartridge_ReadROM(CARTRIDGE *cart, WORD addr);

BYTE Cartridge_ReadRAM(CARTRIDGE *cart, WORD addr);

void Cartridge_WriteRAM(CARTRIDGE *cart, WORD addr, BYTE data);


#endif // CARTRIDGE_H