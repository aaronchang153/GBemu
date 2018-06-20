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

typedef void (*ALU_OP_REG)(CPU*, BYTE*);
typedef void (*ALU_OP_MEM)(CPU*, WORD);
typedef void (*ROT_OP_REG)(CPU*, BYTE*);
typedef void (*ROT_OP_MEM)(CPU*, WORD);

static BYTE *deref_rTable(CPU *c, int index);
static WORD *deref_rpTable(CPU *c, int index);
static WORD *deref_rp2Table(CPU *c, int index);
static BYTE deref_ccTable(int index);
static ALU_OP_REG deref_aluRegTable(int index);
static ALU_OP_MEM deref_aluMemTable(int index);
static ROT_OP_REG deref_rotRegTable(int index);
static ROT_OP_MEM deref_rotMemTable(int index);

// Control
static void NOP(CPU *c);
static void STOP(CPU *c);
static void HALT(CPU *c);
static void DI(CPU *c);
static void EI(CPU *c);
// Jumps
static void JP(CPU *c);
static void JP_cc(CPU *c, BYTE cond);
static void JP_HL(CPU *c);
static void JR(CPU *c);
static void JR_cc(CPU *c, BYTE cond);
// Calls
static void CALL(CPU *c);
static void CALL_cc(CPU *c, BYTE cond);
// Returns
static void RET(CPU *c);
static void RET_cc(CPU *c, BYTE cond);
static void RETI(CPU *c);
// Reset
static void RST(CPU *c, BYTE t);
// 8-bit Loads
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
// 16-bit Loads
static void LD_Imm16toReg16(CPU *c, WORD *reg);
static void LD_SPtoMem16(CPU *c);
static void LD_HLtoSP(CPU *c);
static void LD_SPtoHL(CPU *c, SIGNED_BYTE displace);
static void PUSH(CPU *c, REGISTER *reg);
static void POP(CPU *c, REGISTER *reg);
// 8-bit ALU (all of these operate on and store the result in A)
static void ADDA_Reg8(CPU *c, BYTE *reg);
static void ADDA_Mem8(CPU *c, WORD addr);
static void ADCA_Reg8(CPU *c, BYTE *reg);
static void ADCA_Mem8(CPU *c, WORD addr);
static void SUB_Reg8(CPU *c, BYTE *reg);
static void SUB_Mem8(CPU *c, WORD addr);
static void SBC_Reg8(CPU *c, BYTE *reg);
static void SBC_Mem8(CPU *c, WORD addr);
static void AND_Reg8(CPU *c, BYTE *reg);
static void AND_Mem8(CPU *c, WORD addr);
static void XOR_Reg8(CPU *c, BYTE *reg);
static void XOR_Mem8(CPU *c, WORD addr);
static void OR_Reg8(CPU *c, BYTE *reg);
static void OR_Mem8(CPU *c, WORD addr);
static void CP_Reg8(CPU *c, BYTE *reg);
static void CP_Mem8(CPU *c, WORD addr);
static void INC_Reg8(CPU *c, BYTE *reg);
static void DEC_Reg8(CPU *c, BYTE *reg);
// 16-bit ALU
static void ADD_HL(CPU *c, WORD *reg);
static void ADD_SP(CPU *c, SIGNED_BYTE displace);
static void INC_Reg16(CPU *c, WORD *reg);
static void DEC_Reg16(CPU *c, WORD *reg);
// Rotates and shifts
static void RLC_Reg8(CPU *c, BYTE *reg);
static void RLC_Mem8(CPU *c, WORD addr);
static void RRC_Reg8(CPU *c, BYTE *reg);
static void RRC_Mem8(CPU *c, WORD addr);
static void RL_Reg8(CPU *c, BYTE *reg);
static void RL_Mem8(CPU *c, WORD addr);
static void RR_Reg8(CPU *c, BYTE *reg);
static void RR_Mem8(CPU *c, WORD addr);
static void SLA_Reg8(CPU *c, BYTE *reg);
static void SLA_Mem8(CPU *c, WORD addr);
static void SRA_Reg8(CPU *c, BYTE *reg);
static void SRA_Mem8(CPU *c, WORD addr);
static void SWAP_Reg8(CPU *c, BYTE *reg);
static void SWAP_Mem8(CPU *c, WORD addr);
static void SRL_Reg8(CPU *c, BYTE *reg);
static void SRL_Mem8(CPU *c, WORD addr);
// Bit Operations
static void BIT_Reg8(CPU *c, int bit, BYTE *reg);
static void BIT_Mem8(CPU *c, int bit, WORD addr);
static void SET_Reg8(CPU *c, int bit, BYTE *reg);
static void SET_Mem8(CPU *c, int bit, WORD addr);
static void RES_Reg8(CPU *c, int bit, BYTE *reg);
static void RES_Mem8(CPU *c, int bit, WORD addr);
// Misc.
static void DAA(CPU *c); // Convert A to packed BCD
static void inline CPL(CPU *c); // Complement A
static void CCF(CPU *c); // Complement carry flag
static void SCF(CPU *c); // Set carry flag
static void CB_Prefix(CPU *c);

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
                    RLC_Reg8(c, &c->af.hi);
                    break;
                case 1: // RRCA
                    RRC_Reg8(c, &c->af.hi);
                    break;
                case 2: // RLA
                    RL_Reg8(c, &c->af.hi);
                    break;
                case 3: // RRA
                    RR_Reg8(c, &c->af.hi);
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

static void Decode_X_2(CPU *c){
    // alu[y] r[z]
    int y = Y(c->ir);
    int z = Z(c->ir);
    if(z == 6){
        deref_aluMemTable(y)(c, c->hl.reg);
    }
    else if (z >= 0 && z < 8){
        deref_aluRegTable(y)(c, deref_rTable(c, z));
    }
    else{
        p_undef(c);
    }
}

static void Decode_X_3(CPU *c){
    int y = Y(c->ir);
    int z = Z(c->ir);
    int p = P(c->ir);
    int q = Q(c->ir);

    switch(z){
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

static BYTE deref_ccTable(int index){
    switch(index){
        case 0: return N_FLAG | Z_FLAG;
        case 1: return Z_FLAG;
        case 2: return N_FLAG | C_FLAG;
        case 3: return C_FLAG;
        default: return 0;
    };
}

static ALU_OP_REG deref_aluRegTable(int index){
    switch(index){
        case 0: return &ADDA_Reg8;
        case 1: return &ADCA_Reg8;
        case 2: return &SUB_Reg8;
        case 3: return &SBC_Reg8;
        case 4: return &AND_Reg8;
        case 5: return &XOR_Reg8;
        case 6: return &OR_Reg8;
        case 7: return &CP_Reg8;
        default: return NULL;
    };
}

static ALU_OP_MEM deref_aluMemTable(int index){
    switch(index){
        case 0: return &ADDA_Mem8;
        case 1: return &ADCA_Mem8;
        case 2: return &SUB_Mem8;
        case 3: return &SBC_Mem8;
        case 4: return &AND_Mem8;
        case 5: return &XOR_Mem8;
        case 6: return &OR_Mem8;
        case 7: return &CP_Mem8;
        default: return NULL;
    };
}

static ROT_OP_REG deref_rotRegTable(int index){
    switch(index){
        case 0: return &RLC_Reg8;
        case 1: return &RRC_Reg8;
        case 2: return &RL_Reg8;
        case 3: return &RR_Reg8;
        case 4: return &SLA_Reg8;
        case 5: return &SRA_Reg8;
        case 6: return &SWAP_Reg8;
        case 7: return &SRL_Reg8;
        default: return NULL;
    };
}

static ROT_OP_MEM deref_rotMemTable(int index){
    switch(index){
        case 0: return &RLC_Mem8;
        case 1: return &RRC_Mem8;
        case 2: return &RL_Mem8;
        case 3: return &RR_Mem8;
        case 4: return &SLA_Mem8;
        case 5: return &SRA_Mem8;
        case 6: return &SWAP_Mem8;
        case 7: return &SRL_Mem8;
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
