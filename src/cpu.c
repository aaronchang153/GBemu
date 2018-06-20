#include "cpu.h"

#include <stdio.h>
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

static inline void p_undef(CPU *c) { printf("Undefined opcode %x\n", c->ir); }

typedef void (*ALU_OP)(CPU*, BYTE*, BYTE);
typedef void (*ROT_OP)(CPU*, BYTE*);

static BYTE *deref_rTable(CPU *c, int index);
static WORD *deref_rpTable(CPU *c, int index);
static WORD *deref_rp2Table(CPU *c, int index);
static BYTE deref_ccTable(int index);
static ALU_OP deref_aluTable(int index);
static ROT_OP deref_rotTable(int index);

static void NOP(CPU *c);
static void STOP(CPU *c);
static void HALT(CPU *c);
// Jumps
static void JR(CPU *c);
static void JR_cc(CPU *c, BYTE cond);
// Loads
static void LD_SPtoMem16(CPU *c);
static void LD_Imm16toReg16(CPU *c, WORD *reg);
static void LD_Imm8toReg8(CPU *c, BYTE *reg);
static void LD_Imm8toMem8(CPU *c, WORD addr);
static void LD_AtoMem8(CPU *c, WORD addr);
static void LD_Mem8toA(CPU *c, WORD addr);
static void LDI_toMem8(CPU *c, WORD *addr);
static void LDI_toA(CPU *c, WORD *addr);
static void LDD_toMem8(CPU *c, WORD *addr);
static void LDD_toA(CPU *c, WORD *addr);
static void LD_Reg8toReg8(CPU *c, BYTE *reg1, BYTE *reg2);
static void LD_Mem8toReg8(CPU *c, BYTE *reg, WORD addr);
static void LD_Reg8toMem8(CPU *c, WORD addr, BYTE *reg);
// ALU operations
static void ADD_toReg8(CPU *c, BYTE *reg, BYTE data);
static void ADC_toReg8(CPU *c, BYTE *reg, BYTE data);
static void SUB_toReg8(CPU *c, BYTE *reg, BYTE data);
static void SBC_toReg8(CPU *c, BYTE *reg, BYTE data);
static void AND(CPU *c, BYTE* reg, BYTE data);
static void XOR(CPU *c, BYTE* reg, BYTE data);
static void OR(CPU *c, BYTE* reg, BYTE data);
static void CP(CPU *c, BYTE* reg, BYTE data); // compare reg and data
static void INC_Reg8(CPU *c, BYTE *reg);
static void DEC_Reg8(CPU *c, BYTE *reg);
static void INC_Reg16(CPU *c, BYTE *reg);
static void DEC_Reg16(CPU *c, BYTE *reg);
static void ADD_HL(CPU *c, WORD *reg);
static void ADD_SP(CPU *c, SIGNED_BYTE displace);
// Rotates and shifts
static void RLC(CPU *c, BYTE *reg);
static void RRC(CPU *c, BYTE *reg);
static void RL(CPU *c, BYTE *reg);
static void RR(CPU *c, BYTE *reg);
static void SL(CPU *c, BYTE *reg);
static void SR(CPU *c, BYTE *reg);
static void SWAP(CPU *c, BYTE *reg);
static void SRL(CPU *c, BYTE *reg);
// Misc.
static void DAA(CPU *c); // Convert A to packed BCD
static inline CPL(CPU *c); // Complement A
static void CCF(CPU *c); // Complement carry flag
static void SCF(CPU *c); // Set carry flag

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
            p_undef(c);
    };
}

static void Decode_X_0(CPU *c){
    switch(Z(c->ir)){
        case 0:
            switch(Y(c->ir)){
                case 0: // NOP
                    NOP(c);
                    break;
                case 1: // LD (nn),SP
                    LD_SPtoMem16(c);
                    break;
                case 2: // STOP
                    STOP(c);
                    break;
                case 3: // JR d
                    JR(c);
                    break;
                case 4:
                case 5:
                case 6:
                case 7: // JR cc[y-4],d
                    JR_cc(c, deref_ccTable((Y(c->ir)) - 4));
                    break;
                default:
                    p_undef(c);
            };
            break;
        case 1:
            switch(Q(c->ir)){
                case 0: // LD rp[p],nn
                    LD_Imm16toReg16(c, deref_rpTable(c, P(c->ir)));
                    break;
                case 1: // ADD HL,rp[p]
                    ADD_HL(c, deref_rpTable(c, P(c->ir)));
                    break;
                default:
                    p_undef(c);
            };
            break;
        case 2:
            switch(Q(c->ir)){
                case 0:
                    switch(P(c->ir)){
                        case 0: // LD (BC),A
                            LD_AtoMem8(c, c->bc.reg);
                            break;
                        case 1: // LD (DE),A
                            LD_AtoMem8(c, c->de.reg);
                            break;
                        case 2: // LD (HL+),A
                            LDI_toMem8(c, &c->hl.reg);
                            break;
                        case 3: // LD (HL-),A
                            LDD_toMem8(c, &c->hl.reg);
                            break;
                        default:
                            p_undef(c);
                    };
                    break;
                case 1:
                    switch(P(c->ir)){
                        case 0: // LD A,(BC)
                            LD_Mem8toA(c, c->bc.reg);
                            break;
                        case 1: // LD A,(DE)
                            LD_Mem8toA(c, c->de.reg);
                            break;
                        case 2: // LD A,(HL+)
                            LDI_toA(c, &c->hl.reg);
                            break;
                        case 3: // LD A,(HL-)
                            LDD_toA(c, &c->hl.reg);
                            break;
                        default:
                            p_undef(c);
                    };
                    break;
                default:
                    p_undef(c);
            };
            break;
        case 3:
            switch(Q(c->ir)){
                case 0: // INC rp[p]
                    INC_Reg16(c, deref_rpTable(c, P(c->ir)));
                    break;
                case 1: // DEC rp[p]
                    DEC_Reg16(c, deref_rpTable(c, P(c->ir)));
                    break;
                default:
                    p_undef(c);
            };
            break;
        case 4: // INC r[y]
            switch(Y(c->ir)){
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 7:
                    INC_Reg8(c, deref_rTable(c, Y(c->ir)));
                    break;
                case 6:
                    Mem_IncByte(c->memory, c->hl.reg);
                    break;
                default:
                    p_undef(c);
            };
            break;
        case 5: // DEC r[y]
            switch(Y(c->ir)){
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 7:
                    DEC_Reg8(c, deref_rTable(c, Y(c->ir)));
                    break;
                case 6:
                    Mem_DecByte(c->memory, c->hl.reg);
                    break;
                default:
                    p_undef(c);
            };
        case 6: // LD r[y],n
            switch(Y(c->ir)){
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 7:
                    LD_Imm8toReg8(c, deref_rTable(c, Y(c->ir)));
                    break;
                case 6:
                    LD_Imm8toMem8(c, c->hl.reg);
                    break;
                default:
                    p_undef(c);
            };
            break;
        case 7:
            switch(Y(c->ir)){
                case 0: // RLCA
                    RLC(c, &c->af.hi);
                    break;
                case 1: // RRCA
                    RRC(c, &c->af.hi);
                    break;
                case 2: // RLA
                    RL(c, &c->af.hi);
                    break;
                case 3: // RRA
                    RR(c, &c->af.hi);
                    break;
                case 4: // DAA
                    DAA(c);
                    break;
                case 5: // CPL
                    CPL(c);
                    break;
                case 6: // SCF
                    SCF(c);
                    break;
                case 7: // CCF
                    CCF(c);
                    break;
                default:
                    p_undef(c);
            };
            break;
        default:
            p_undef(c);
    };
}

static void Decode_X_1(CPU *c){
    int z = Z(c->ir);
    int y = Y(c->ir);
    if(y == 6 && z == 6){ // HALT
        HALT(c);
    }
    else{ // LD r[y],r[z]
        if(z == 6){
            LD_Mem8toReg8(c, deref_rTable(c, y), c->hl.reg);
        }
        else{
            if(y == 6){
                LD_Reg8toMem8(c, c->hl.reg, deref_rTable(c, z));
            }
            else{
                LD_Reg8toReg8(c, deref_rTable(c, y), deref_rTable(c, z));
            }
        }
    }
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

static BYTE deref_ccTable(int index){
    switch(index){
        case 0: return N_FLAG | Z_FLAG;
        case 1: return Z_FLAG;
        case 2: return N_FLAG | C_FLAG;
        case 3: return C_FLAG;
        default: return 0;
    };
}

static ALU_OP deref_aluTable(int index){
    switch(index){
        case 0: return &ADD_toReg8;
        case 1: return &ADC_toReg8;
        case 2: return &SUB_toReg8;
        case 3: return &SBC_toReg8;
        case 4: return &AND;
        case 5: return &XOR;
        case 6: return &OR;
        case 7: return &CP;
        default: return NULL;
    };
}

static ROT_OP deref_rotTable(int index){
    switch(index){
        case 0: return &RLC;
        case 1: return &RRC;
        case 2: return &RL;
        case 3: return &RR;
        case 4: return &SL;
        case 5: return &SR;
        case 6: return &SWAP;
        case 7: return &SRL;
        default: return NULL;
    };
}

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
