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
    printf("FLAGS (Register F expanded):\n");
    printf("\tZ: %d\n", CPU_CheckFlag(c, Z_FLAG));
    printf("\tN: %d\n", CPU_CheckFlag(c, N_FLAG));
    printf("\tH: %d\n", CPU_CheckFlag(c, H_FLAG));
    printf("\tC: %d\n", CPU_CheckFlag(c, C_FLAG));
    printf("MISC:\n");
    printf("\tIMM8:  0x%02x\n", Mem_ReadByte(c->memory, c->pc+1));
    printf("\tIMM16: 0x%04x\n", Mem_ReadWord(c->memory, c->pc+1));
    //printf("\tMEM[HL-1]: 0x%02x\n", Mem_ReadByte(c->memory, c->hl.reg-1));
    printf("\tMEM[HL]  : 0x%02x\n", Mem_ReadByte(c->memory, c->hl.reg));
    //printf("\tMEM[HL+1]: 0x%02x\n", Mem_ReadByte(c->memory, c->hl.reg+1));
    printf("==================================================\n");
}

void Enter_Debug_Mode(CPU *c){
    while(true){
        CPU_Fetch(c);
        Print_CPU_State(c);
        CPU_DecodeExecute(c);
        if(getchar() == 'Q'){
            break;
        }
    }
}

void Start_Debugger(CPU *c){
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