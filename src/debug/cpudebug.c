#include "cpudebug.h"

#ifdef DEBUG

#include "disassemble.h"

#include <stdio.h>
#include <stdlib.h>

static void Print_CPU_State(CPU *c){
    printf("==================================================\n");
    printf("OPCODE: 0x%02x\t\tDISASSEMBLY: %s\n", c->ir, Disassemble_Instruction(c));
    printf("REGISTERS:\n");
    printf("\tPC: 0x%04x\n", c->pc);
    printf("\tSP: 0x%04x\n", c->sp);
    printf("\tA: 0x%02x\t\tF: 0x%02x\n", c->af.hi, c->af.lo);
    printf("\tB: 0x%02x\t\tC: 0x%02x\n", c->bc.hi, c->bc.lo);
    printf("\tD: 0x%02x\t\tE: 0x%02x\n", c->de.hi, c->de.lo);
    printf("\tH: 0x%02x\t\tL: 0x%02x\n", c->hl.hi, c->hl.lo);
    printf("FLAGS:\n");
    printf("\tZ:   %d\n", CPU_CheckFlag(c, Z_FLAG));
    printf("\tN:   %d\n", CPU_CheckFlag(c, N_FLAG));
    printf("\tH:   %d\n", CPU_CheckFlag(c, H_FLAG));
    printf("\tC:   %d\n", CPU_CheckFlag(c, C_FLAG));
    printf("\tIME: %d\n", c->IME);
    printf("MISC:\n");
    printf("\tIMM8:  0x%02x\n", Mem_ReadByte(c->memory, c->pc+1));
    printf("\tIMM16: 0x%04x\n", Mem_ReadWord(c->memory, c->pc+1));
    //printf("\tMEM[HL-1]: 0x%02x\n", Mem_ReadByte(c->memory, c->hl.reg-1));
    printf("\tMEM[HL]  : 0x%02x\n", Mem_ReadByte(c->memory, c->hl.reg));
    //printf("\tMEM[HL+1]: 0x%02x\n", Mem_ReadByte(c->memory, c->hl.reg+1));
    printf("==================================================\n");
}

static void Dump_Memory(CPU *c){
    FILE *fp = fopen("Mem_Dump.txt", "w");
    MEM_REGION region;
    BYTE content;
    if(fp != NULL){
        for(int i = 0; i < 0x10000; i++){
            region = Mem_GetRegion(c->memory, (WORD) i);
            if(region == ROM0 || region == ROMX)
                content = (c->memory->cartridge == NULL) ? 0 : Cartridge_ReadROM(c->memory->cartridge, i);
            else if(region == SRAM)
                content = (c->memory->cartridge == NULL) ? 0 : Cartridge_ReadRAM(c->memory->cartridge, i);
            else
                content = Mem_ReadByte(c->memory, i);
            fprintf(fp, "0x%04x:\t0x%02x\n", i, content);
        }
        fclose(fp);
        printf("Memory successfully dumped.\n");
    }
}

void Enter_Debug_Mode(CPU *c){
    char input;
    while(true){
        CPU_Fetch(c);
        Print_CPU_State(c);
        input = getchar();
        if(input == 'q'){
            break;
        }
        else if(input == 'D'){
            Dump_Memory(c);
        }
        CPU_DecodeExecute(c);
    }
}

void Start_CPU_Debugger(CPU *c){
    int mode;
    int bp;
    printf("Enter debug mode: [1] Line-By-Line [2] Break [3] Normal\n");
    scanf("%d", &mode);
    if(mode == 1){
        Enter_Debug_Mode(c);
    }
    else if(mode == 2){
        printf("Break at PC = ");
        scanf("%d", &bp);
        while(c->pc != bp){
            CPU_EmulateCycle(c);
        }
        Enter_Debug_Mode(c);
    }
    else if(mode == 3){
        while(true){
            CPU_EmulateCycle(c);
        }
    }
}

#endif // DEBUG