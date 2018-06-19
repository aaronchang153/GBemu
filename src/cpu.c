#include "cpu.h"

#include <stdlib.h>
#include <string.h>

/*
 * Decoding algorithm taken from https://avivace.com/apps/gbdev/decoding_gz80.html
 * 
 * Opcode:
 * |7 6|5 4 3|2 1 0|
 * |-x-|--y--|--z--|
 *     |-p-|q|
 * 
 */

#define X(op) (op & 0xC0) >> 6
#define Y(op) (op & 0x38) >> 3
#define Z(op) (op & 0x07)
#define P(op) (op & 0x30) >> 4
#define Q(op) (op & 0x08) >> 3

static void Decode_X_0(CPU *c);
static void Decode_X_1(CPU *c);
static void Decode_X_2(CPU *c);
static void Decode_X_3(CPU *c);

typedef void (*ALU_OP)(CPU *c);

static BYTE *deref_rTable(CPU *c, int index);
static WORD *deref_rpTable(CPU *c, int index);
static WORD *deref_rp2Table(CPU *c, int index);
static BYTE deref_ccTable(int index);
static ALU_OP deref_aluTable(int index);
static ALU_OP deref_rotTable(int index);

static void NOP(CPU *c);
static void STOP(CPU *c);
static void JR(CPU *c);
static void JR_cc(CPU *c, BYTE cond);
static void LD_toMem8(CPU *c, WORD addr, BYTE data);
static void LD_toReg8(CPU *c, BYTE *reg, BYTE data);
static void LD_toMem16(CPU *c, WORD addr, WORD data);
static void LD_toReg16(CPU *c, WORD *reg, WORD data);
static void INC_Reg8(CPU *c, BYTE *reg);
static void DEC_Reg8(CPU *c, BYTE *reg);

static const BYTE Z_FLAG = 0x80; // Zero flag
static const BYTE N_FLAG = 0x40; // Subtract flag
static const BYTE H_FLAG = 0x20; // Half-Carry flag
static const BYTE C_FLAG = 0x10; // Carry flag


CPU *CPU_Create(){
    CPU *cpu = NULL;
    cpu = (CPU *) malloc(sizeof(CPU));
    if(cpu != NULL){
        cpu->memory = Mem_Create();
        if(cpu->memory == NULL){
            CPU_Destroy(cpu);
            cpu = NULL;
        }
    }
    return cpu;
}

// Initialization handled by bootstrap program

/*
void CPU_Init(CPU *c){
    if(c != NULL){
        c->sp = 0xFFFE;
    }
}
*/

void CPU_Destroy(CPU *c){
    if(c != NULL){
        Mem_Destroy(c->memory);
        free(c);
    }
}

void CPU_EmulateCycle(CPU *c){
    c->ir = Mem_ReadByte(c->memory, c->pc);
    CPU_DecodeExecute(c);
}

void CPU_DecodeExecute(CPU *c){
    switch(X(c->ir)){
        case 0:
            Decode_X_0(c);
            break;
        case 1:
            Decode_X_1(c);
            break;
        case 2:
            Decode_X_2(c);
            break;
        case 3:
            Decode_X_3(c);
            break;
        default:
            printf("Undefined opcode %x\n", c->ir);
    };
}

static void Decode_X_0(CPU *c){
    switch(Z(c->ir)){
        case 0:
            break;
    };
}

static BYTE *deref_rTable(CPU *c, int index){
    switch(index){
        case 0: return &c->bc.hi;
        case 1: return &c->bc.lo;
        case 2: return &c->de.hi;
        case 3: return &c->de.lo;
        case 4: return &c->hl.hi;
        case 5: return &c->hl.lo;
        case 7: return &c->af.hi;
        default: return NULL;
    };
}

static WORD *deref_rpTable(CPU *c, int index){
    switch(index){
        case 0: return &c->bc.reg;
        case 1: return &c->de.reg;
        case 2: return &c->hl.reg;
        case 3: return &c->sp;
        default: return NULL;
    };
}

static WORD *deref_rp2Table(CPU *c, int index){
    switch(index){
        case 0: return &c->bc.reg;
        case 1: return &c->de.reg;
        case 2: return &c->hl.reg;
        case 3: return &c->af.reg;
        default: return NULL;
    };
}

static BYTE deref_ccTable(int index);

static ALU_OP deref_aluTable(int index);

static ALU_OP deref_rotTable(int index);

static void INC_Reg8(CPU *c, BYTE *reg){
    if((*reg & 0x0F) == 0x0F)
        CPU_SetFlag(c, H_FLAG);
    else
        CPU_ClearFlag(c, H_FLAG);
    *reg++;
    if(*reg == 0)
        CPU_SetFlag(c, Z_FLAG);
    else
        CPU_ClearFlag(c, Z_FLAG);
    CPU_ClearFlag(c, N_FLAG);
}

static void DEC_Reg8(CPU *c, BYTE *reg){
    if((*reg & 0x0F) == 0)
        CPU_SetFlag(c, H_FLAG);
    else
        CPU_ClearFlag(c, H_FLAG);
    *reg--;
    if(*reg == 0)
        CPU_SetFlag(c, Z_FLAG);
    else
        CPU_ClearFlag(c, Z_FLAG);
    CPU_SetFlag(c, N_FLAG);
}
