#include "opcode.h"

#include <stdio.h>

#define CYCLES(n) 4 * n

typedef unsigned int uint32_t;

static inline BYTE HI(WORD w) { return (BYTE) ((w & 0xFF00) >> 8); }
static inline BYTE LO(WORD w) { return (BYTE)  (w & 0x00FF); }

static inline BYTE IMM8(CPU *c) { return Mem_ReadByte(c->memory, c->pc+1); }
static inline WORD IMM16(CPU *c) { return Mem_ReadWord(c->memory, c->pc+1); }

static inline bool HALF_CARRY_ADD(BYTE a, BYTE b) { return (((a & 0x0F) + (b & 0x0F)) & 0xF0) == 0x10; }
static inline bool CARRY_ADD(BYTE a, BYTE b) { return (((WORD) a + (WORD) b) & 0xFF00) == 0x0100; }

static inline bool HALF_CARRY_SUB(BYTE a, BYTE b) { return (a & 0x0F) < (b & 0x0F); }
static inline bool CARRY_SUB(BYTE a, BYTE b) { return a < b; }

static inline bool HALF_CARRY_ADD16(WORD a, WORD b) { return (((a & 0x0FFF) + (b & 0x0FFF)) & 0xF000) == 0x1000; }
static inline bool CARRY_ADD16(WORD a, WORD b) { return (((uint32_t) a + (uint32_t) b) & 0xF0000) == 0x10000; }


// Control
void NOP(CPU *c){
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void STOP(CPU *c){
    printf("STOP: Unimplemented\n");
    // prereq: - all interrupt-enable flags (IE) are reset
    //         - input to P10-P13 is low for all
}

void HALT(CPU *c){
    printf("HALT: Unimplemented\n");
}

void DI(CPU *c){
    c->IME = false;
    c->pc++;
}

void EI(CPU *c){
    printf("EI: Unimplemented\n");
}

// Restart
void RST(CPU *c){
    printf("RST: Unimplemented\n");
}

// Jumps
void JP(CPU *c){
    c->pc = IMM16(c);
    CPU_SetCycles(c, CYCLES(4));
}

void JP_cc(CPU *c, BYTE cond, bool not){
    if(CPU_CheckFlag(c, cond) != not){
        c->pc = IMM16(c);
        CPU_SetCycles(c, CYCLES(4));
    }
    else{
        c->pc += 3;
        CPU_SetCycles(c, CYCLES(3));
    }
}

void JP_HL(CPU *c){
    c->pc = c->hl.reg;
    CPU_SetCycles(c, CYCLES(1));
}

void JR(CPU *c){
    c->pc = c->pc + 2 + (SIGNED_BYTE) IMM8(c);
    CPU_SetCycles(c, CYCLES(3));
}

void JR_cc(CPU *c, BYTE cond, bool not){
    if(CPU_CheckFlag(c, cond) != not){
        c->pc = c->pc + 2 + (SIGNED_BYTE) IMM8(c);
        CPU_SetCycles(c, CYCLES(3));
    }
    else{
        c->pc += 2;
        CPU_SetCycles(c, CYCLES(2));
    }
}

// Calls
void CALL(CPU *c){
    Mem_WriteByte(c->memory, c->sp-1, HI(c->pc));
    Mem_WriteByte(c->memory, c->sp-2, LO(c->pc));
    c->pc = IMM16(c);
    c->sp -= 2;
    CPU_SetCycles(c, CYCLES(6));
}

void CALL_cc(CPU *c, BYTE cond, bool not){
    if(CPU_CheckFlag(c, cond) != not){
        Mem_WriteByte(c->memory, c->sp-1, HI(c->pc));
        Mem_WriteByte(c->memory, c->sp-2, LO(c->pc));
        c->pc = IMM16(c);
        c->sp -= 2;
        CPU_SetCycles(c, CYCLES(6));
    }
    else{
        c->pc += 3;
        CPU_SetCycles(c, CYCLES(3));
    }
}

// Returns
void RET(CPU *c){
    c->pc = Mem_ReadWord(c->memory, c->sp);
    c->sp += 2;
    CPU_SetCycles(c, CYCLES(4));
}

void RET_cc(CPU *c, BYTE cond, bool not){
    if(CPU_CheckFlag(c, cond) != not){
        c->pc = Mem_ReadWord(c->memory, c->sp);
        c->sp += 2;
        CPU_SetCycles(c, CYCLES(5));
    }
    else{
        c->pc++;
        CPU_SetCycles(c, CYCLES(2));
    }
}

void RETI(CPU *c){
    printf("RETI: Unimplemented\n");
}

// 8-bit Loads
void LD_Imm8toReg8(CPU *c, BYTE *reg){
    *reg = IMM8(c);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void LD_Imm8toMem8(CPU *c, WORD addr){
    Mem_WriteByte(c->memory, addr, IMM8(c));
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(3));
}

void LD_AtoMem8_addr(CPU *c, WORD addr){
    Mem_WriteByte(c->memory, addr, c->af.hi);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void LD_AtoMem8_imm(CPU *c){
    Mem_WriteByte(c->memory, IMM16(c), c->af.hi);
    c->pc += 3;
    CPU_SetCycles(c, CYCLES(4));
}

void LD_Mem8toA_addr(CPU *c, WORD addr){
    c->af.hi = Mem_ReadByte(c->memory, addr);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void LD_Mem8toA_imm(CPU *c){
    c->af.hi = Mem_ReadByte(c->memory, IMM16(c));
    c->pc += 3;
    CPU_SetCycles(c, CYCLES(4));
}

void LDI_toMem8(CPU *c){
    Mem_WriteByte(c->memory, c->hl.reg, c->af.hi);
    c->hl.reg++;
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void LDI_toA(CPU *c){
    c->af.hi = Mem_ReadByte(c->memory, c->hl.reg);
    c->hl.reg++;
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void LDD_toMem8(CPU *c){
    Mem_WriteByte(c->memory, c->hl.reg, c->af.hi);
    c->hl.reg--;
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void LDD_toA(CPU *c){
    c->af.hi = Mem_ReadByte(c->memory, c->hl.reg);
    c->hl.reg--;
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void LD_Reg8toReg8(CPU *c, BYTE *reg1, BYTE *reg2){
    *reg1 = *reg2;
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void LD_Mem8toReg8_addr(CPU *c, BYTE *reg, WORD addr){
    // Only used to read from (HL)
    *reg = Mem_ReadByte(c->memory, addr);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

// void LD_Mem8toReg8_imm(CPU *c, BYTE *reg); >> Not needed

void LD_Reg8toMem8_addr(CPU *c, WORD addr, BYTE *reg){
    // Only used to write to (HL)
    Mem_WriteByte(c->memory, addr, *reg);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

// void LD_Reg8toMem8_imm(CPU *c, BYTE *reg); // Not needed

void LD_CtoA(CPU *c){
    c->af.hi = Mem_ReadByte(c->memory, 0xFF00 + c->bc.lo);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void LD_AtoC(CPU *c){
    Mem_WriteByte(c->memory, 0xFF00 + c->bc.lo, c->af.hi);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void LDH_NtoA(CPU *c){
    c->af.hi = Mem_ReadByte(c->memory, 0xFF00 + IMM8(c));
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(3));
}

void LDH_AtoN(CPU *c){
    Mem_WriteByte(c->memory, 0xFF00 + IMM8(c), c->af.hi);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(3));
}

// 16-bit Loads
void LD_Imm16toReg16(CPU *c, WORD *reg){
    *reg = IMM16(c);
    c->pc += 3;
    CPU_SetCycles(c, CYCLES(3));
}

void LD_SPtoMem16(CPU *c){
    WORD addr = IMM16(c);
    Mem_WriteByte(c->memory, addr, LO(c->sp));
    Mem_WriteByte(c->memory, addr + 1, HI(c->sp));
    c->pc += 3;
    CPU_SetCycles(c, CYCLES(5));
}

void LD_HLtoSP(CPU *c){
    c->sp = c->hl.reg;
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void LD_SPtoHL(CPU *c){
    // e MAY NEED TO BE SIGNED + SIGN EXTENDED
    BYTE e = IMM8(c);
    CPU_ClearFlag(c, Z_FLAG | N_FLAG);
    if(((c->sp & 0x0FFF) + (WORD) e) == 0x1000){
        // Set H if there's a carry from bit 11
        CPU_SetFlag(c, H_FLAG, true);
    }
    else{
        CPU_ClearFlag(c, H_FLAG);
    }
    if((((unsigned int) c->sp) + (unsigned int) e) == 0x10000){
        // Set C if there's a carry from bit 15
        CPU_SetFlag(c, C_FLAG, true);
    }
    else{
        CPU_ClearFlag(c, C_FLAG);
    }
    c->hl.reg = c->sp + (SIGNED_BYTE) e;
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(3));
}

void PUSH(CPU *c, WORD *reg){
    Mem_WriteByte(c->memory, c->sp - 1, HI(*reg));
    Mem_WriteByte(c->memory, c->sp - 2, LO(*reg));
    c->sp -= 2;
    c->pc++;
    CPU_SetCycles(c, CYCLES(4));
}

void POP(CPU *c, WORD *reg){
    *reg = Mem_ReadWord(c->memory, c->sp);
    c->sp += 2;
    c->pc++;
    CPU_SetCycles(c, CYCLES(3));
}

// 8-bit ALU (all of these operate on and store the result in A)
void ADDA_Reg8(CPU *c, BYTE *reg){
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_ADD(c->af.hi, *reg));
    CPU_SetFlag(c, C_FLAG, CARRY_ADD(c->af.hi, *reg));
    CPU_SetFlag(c, Z_FLAG, (c->af.hi += *reg) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void ADDA_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_ADD(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, CARRY_ADD(c->af.hi, data));
    CPU_SetFlag(c, Z_FLAG, (c->af.hi += data) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void ADDA_Imm8(CPU *c){
    BYTE data = IMM8(c);
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_ADD(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, CARRY_ADD(c->af.hi, data));
    CPU_SetFlag(c, Z_FLAG, (c->af.hi += data) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void ADCA_Reg8(CPU *c, BYTE *reg){
    bool cy = CPU_CheckFlag(c, C_FLAG);
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_ADD(c->af.hi, *reg));
    CPU_SetFlag(c, C_FLAG, CARRY_ADD(c->af.hi, *reg));
    c->af.hi += *reg;
    if(cy){
        if(HALF_CARRY_ADD(c->af.hi, 1)){
            CPU_SetFlag(c, H_FLAG, true);
        }
        if(CARRY_ADD(c->af.hi, 1)){
            CPU_SetFlag(c, C_FLAG, true);
        }
        c->af.hi++;
    }
    CPU_SetFlag(c, Z_FLAG, c->af.hi == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void ADCA_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    bool cy = CPU_CheckFlag(c, C_FLAG);
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_ADD(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, CARRY_ADD(c->af.hi, data));
    c->af.hi += data;
    if(cy){
        if(HALF_CARRY_ADD(c->af.hi, 1)){
            CPU_SetFlag(c, H_FLAG, true);
        }
        if(CARRY_ADD(c->af.hi, 1)){
            CPU_SetFlag(c, C_FLAG, true);
        }
        c->af.hi++;
    }
    CPU_SetFlag(c, Z_FLAG, c->af.hi == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void ADCA_Imm8(CPU *c){
    BYTE data = IMM8(c);
    bool cy = CPU_CheckFlag(c, C_FLAG);
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_ADD(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, CARRY_ADD(c->af.hi, data));
    c->af.hi += data;
    if(cy){
        if(HALF_CARRY_ADD(c->af.hi, 1)){
            CPU_SetFlag(c, H_FLAG, true);
        }
        if(CARRY_ADD(c->af.hi, 1)){
            CPU_SetFlag(c, C_FLAG, true);
        }
        c->af.hi++;
    }
    CPU_SetFlag(c, Z_FLAG, c->af.hi == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void SUB_Reg8(CPU *c, BYTE *reg){
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_SUB(c->af.hi, *reg));
    CPU_SetFlag(c, C_FLAG, CARRY_SUB(c->af.hi, *reg));
    CPU_SetFlag(c, Z_FLAG, (c->af.hi -= *reg) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void SUB_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_SUB(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, CARRY_SUB(c->af.hi, data));
    CPU_SetFlag(c, Z_FLAG, (c->af.hi -= data) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void SUB_Imm8(CPU *c){
    BYTE data = IMM8(c);
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_SUB(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, CARRY_SUB(c->af.hi, data));
    CPU_SetFlag(c, Z_FLAG, (c->af.hi -= data) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void SBC_Reg8(CPU *c, BYTE *reg){
    bool cy = CPU_CheckFlag(c, C_FLAG);
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_SUB(c->af.hi, *reg));
    CPU_SetFlag(c, C_FLAG, CARRY_SUB(c->af.hi, *reg));
    c->af.hi -= *reg;
    if(cy){
        if(HALF_CARRY_SUB(c->af.hi, 1)){
            CPU_SetFlag(c, H_FLAG, true);
        }
        if(CARRY_SUB(c->af.hi, 1)){
            CPU_SetFlag(c, C_FLAG, true);
        }
        c->af.hi--;
    }
    CPU_SetFlag(c, Z_FLAG, c->af.hi == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void SBC_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    bool cy = CPU_CheckFlag(c, C_FLAG);
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_SUB(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, CARRY_SUB(c->af.hi, data));
    c->af.hi -= data;
    if(cy){
        if(HALF_CARRY_SUB(c->af.hi, 1)){
            CPU_SetFlag(c, H_FLAG, true);
        }
        if(CARRY_SUB(c->af.hi, 1)){
            CPU_SetFlag(c, C_FLAG, true);
        }
        c->af.hi--;
    }
    CPU_SetFlag(c, Z_FLAG, c->af.hi == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void SBC_Imm8(CPU *c){
    BYTE data = IMM8(c);
    bool cy = CPU_CheckFlag(c, C_FLAG);
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_SUB(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, CARRY_SUB(c->af.hi, data));
    c->af.hi -= data;
    if(cy){
        if(HALF_CARRY_SUB(c->af.hi, 1)){
            CPU_SetFlag(c, H_FLAG, true);
        }
        if(CARRY_SUB(c->af.hi, 1)){
            CPU_SetFlag(c, C_FLAG, true);
        }
        c->af.hi--;
    }
    CPU_SetFlag(c, Z_FLAG, c->af.hi == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void AND_Reg8(CPU *c, BYTE *reg){
    CPU_ClearFlag(c, C_FLAG | N_FLAG);
    CPU_SetFlag(c, H_FLAG, true);
    CPU_SetFlag(c, Z_FLAG, (c->af.hi &= *reg) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void AND_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    CPU_ClearFlag(c, C_FLAG | N_FLAG);
    CPU_SetFlag(c, H_FLAG, true);
    CPU_SetFlag(c, Z_FLAG, (c->af.hi &= data) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void AND_Imm8(CPU *c){
    BYTE data = IMM8(c);
    CPU_ClearFlag(c, C_FLAG | N_FLAG);
    CPU_SetFlag(c, H_FLAG, true);
    CPU_SetFlag(c, Z_FLAG, (c->af.hi &= data) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void XOR_Reg8(CPU *c, BYTE *reg){
    CPU_ClearFlag(c, C_FLAG | H_FLAG | N_FLAG);
    CPU_SetFlag(c, Z_FLAG, (c->af.hi ^= *reg) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void XOR_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    CPU_ClearFlag(c, C_FLAG | H_FLAG | N_FLAG);
    CPU_SetFlag(c, Z_FLAG, (c->af.hi ^= data) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void XOR_Imm8(CPU *c){
    BYTE data = IMM8(c);
    CPU_ClearFlag(c, C_FLAG | H_FLAG | N_FLAG);
    CPU_SetFlag(c, Z_FLAG, (c->af.hi ^= data) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void OR_Reg8(CPU *c, BYTE *reg){
    CPU_ClearFlag(c, C_FLAG | H_FLAG | N_FLAG);
    CPU_SetFlag(c, Z_FLAG, (c->af.hi |= *reg) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void OR_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    CPU_ClearFlag(c, C_FLAG | H_FLAG | N_FLAG);
    CPU_SetFlag(c, Z_FLAG, (c->af.hi |= data) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void OR_Imm8(CPU *c){
    BYTE data = IMM8(c);
    CPU_ClearFlag(c, C_FLAG | H_FLAG | N_FLAG);
    CPU_SetFlag(c, Z_FLAG, (c->af.hi |= data) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void CP_Reg8(CPU *c, BYTE *reg){
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, c->af.hi < *reg);
    CPU_SetFlag(c, Z_FLAG, c->af.hi == *reg);
    CPU_SetFlag(c, C_FLAG, c->af.hi > *reg);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void CP_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, c->af.hi < data);
    CPU_SetFlag(c, Z_FLAG, c->af.hi == data);
    CPU_SetFlag(c, C_FLAG, c->af.hi > data);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void CP_Imm8(CPU *c){
    BYTE data = IMM8(c);
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, c->af.hi < data);
    CPU_SetFlag(c, Z_FLAG, c->af.hi == data);
    CPU_SetFlag(c, C_FLAG, c->af.hi > data);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void INC_Reg8(CPU *c, BYTE *reg){
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_ADD(*reg, 1));
    CPU_SetFlag(c, Z_FLAG, ++(*reg) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void INC_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_ADD(data, 1));
    CPU_SetFlag(c, Z_FLAG, ++data == 0);
    Mem_WriteByte(c->memory, addr, data);
    c->pc++;
    CPU_SetCycles(c, CYCLES(3));
}

void DEC_Reg8(CPU *c, BYTE *reg){
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_SUB(*reg, 1));
    CPU_SetFlag(c, Z_FLAG, --(*reg) == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void DEC_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_SUB(data, 1));
    CPU_SetFlag(c, Z_FLAG, --data == 0);
    Mem_WriteByte(c->memory, addr, data);
    c->pc++;
    CPU_SetCycles(c, CYCLES(3));
}

// 16-bit ALU
void ADD_HL(CPU *c, WORD *reg){
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_ADD16(c->hl.reg, *reg));
    CPU_SetFlag(c, C_FLAG, CARRY_ADD16(c->hl.reg, *reg));
    c->hl.reg += (*reg);
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void ADD_SP(CPU *c){
    // DATA MAY NEED TO BE SIGNED AND SIGN EXTENDED
    BYTE data = IMM8(c);
    CPU_ClearFlag(c, N_FLAG | Z_FLAG);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_ADD16(c->sp, (WORD) data));
    CPU_SetFlag(c, C_FLAG, CARRY_ADD16(c->sp, (WORD) data));
    c->sp += (SIGNED_WORD) data;
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(4));
}

void INC_Reg16(CPU *c, WORD *reg){
    (*reg)++;
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void DEC_Reg16(CPU *c, WORD *reg){
    (*reg)--;
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

// Rotates and shifts
void RLCA(CPU *c){
    BYTE rotate;
    CPU_ClearFlag(c, H_FLAG | N_FLAG | Z_FLAG);
    CPU_SetFlag(c, C_FLAG, (rotate = c->af.hi >> 7) == 1);
    c->af.hi = (c->af.hi << 1) | rotate;
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void RLA(CPU *c){
    BYTE rotate = (CPU_CheckFlag(c, C_FLAG)) ? 0x01 : 0x00;
    CPU_ClearFlag(c, H_FLAG | N_FLAG | Z_FLAG);
    CPU_SetFlag(c, C_FLAG, (c->af.hi >> 7) == 1);
    c->af.hi = (c->af.hi << 1) | rotate;
    c->pc++;
    CPU_SetCycles(c, CYCLES(2));
}

void RRCA(CPU *c){
    BYTE rotate;
    CPU_ClearFlag(c, H_FLAG | N_FLAG | Z_FLAG);
    CPU_SetFlag(c, C_FLAG, (rotate = c->af.hi & 0x01) == 1);
    c->af.hi = (c->af.hi >> 1) | (rotate << 7);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void RRA(CPU *c){
    BYTE rotate = (CPU_CheckFlag(c, C_FLAG)) ? 0x80 : 0x00;
    CPU_ClearFlag(c, H_FLAG | N_FLAG | Z_FLAG);
    CPU_SetFlag(c, C_FLAG, (c->af.hi & 0x01) == 1);
    c->af.hi = (c->af.hi >> 1) | rotate;
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

// CB Prefixed Rotates and Shifts
void RLC_Reg8(CPU *c, BYTE *reg){
    BYTE rotate;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, (rotate = (*reg) >> 7) == 1);
    CPU_SetFlag(c, Z_FLAG, (*reg = ((*reg) << 1) | rotate) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void RLC_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    BYTE rotate;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, (rotate = data >> 7) == 1);
    CPU_SetFlag(c, Z_FLAG, ((data = data << 1) | rotate) == 0);
    Mem_WriteByte(c->memory, addr, data);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(4));
}

void RRC_Reg8(CPU *c, BYTE *reg){
    BYTE rotate;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, (rotate = (*reg) & 0x01) == 1);
    CPU_SetFlag(c, Z_FLAG, (*reg = ((*reg) >> 1) | (rotate << 7)) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void RRC_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    BYTE rotate;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, (rotate = data & 0x01) == 1);
    CPU_SetFlag(c, Z_FLAG, (data = (data >> 1) | (rotate << 7)) == 0);
    Mem_WriteByte(c->memory, addr, data);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(4));
}

void RL_Reg8(CPU *c, BYTE *reg){
    BYTE rotate = (CPU_CheckFlag(c, C_FLAG)) ? 0x01 : 0x00;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, ((*reg) >> 7) == 1);
    CPU_SetFlag(c, Z_FLAG, (*reg = ((*reg) << 1) | rotate) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void RL_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    BYTE rotate = (CPU_CheckFlag(c, C_FLAG)) ? 0x01 : 0x00;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, (data >> 7) == 1);
    CPU_SetFlag(c, Z_FLAG, (data = (data << 1) | rotate) == 0);
    Mem_WriteByte(c->memory, addr, data);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(4));
}

void RR_Reg8(CPU *c, BYTE *reg){
    BYTE rotate = (CPU_CheckFlag(c, C_FLAG)) ? 0x80 : 0x00;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, ((*reg) & 0x01) == 1);
    CPU_SetFlag(c, Z_FLAG, (*reg = ((*reg) >> 1) | rotate) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void RR_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    BYTE rotate = (CPU_CheckFlag(c, C_FLAG)) ? 0x80 : 0x00;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, (data & 0x01) == 1);
    CPU_SetFlag(c, Z_FLAG, (data = (data >> 1) | rotate) == 0);
    Mem_WriteByte(c->memory, addr, data);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(4));
}

void SLA_Reg8(CPU *c, BYTE *reg){
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, ((*reg) & 0x80) == 0x80);
    CPU_SetFlag(c, Z_FLAG, ((*reg) <<= 1) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void SLA_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, (data & 0x80) == 0x80);
    CPU_SetFlag(c, Z_FLAG, (data <<= 1) == 0);
    Mem_WriteByte(c->memory, addr, data);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(4));
}

void SRA_Reg8(CPU *c, BYTE *reg){
    BYTE extend = (*reg) & 0x80;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, ((*reg) & 0x01) == 0x01);
    CPU_SetFlag(c, Z_FLAG, (*reg = ((*reg) >> 1) | extend) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void SRA_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    BYTE extend = data & 0x80;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, (data & 0x01) == 0x01);
    CPU_SetFlag(c, Z_FLAG, (data = (data >> 1) | extend) == 0);
    Mem_WriteByte(c->memory, addr, data);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(4));
}

void SWAP_Reg8(CPU *c, BYTE *reg){
    CPU_ClearFlag(c, C_FLAG | H_FLAG | N_FLAG);
    CPU_SetFlag(c, Z_FLAG, (*reg = (((*reg) & 0x0F) << 4) | (((*reg) & 0xF0) >> 4)) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void SWAP_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    CPU_ClearFlag(c, C_FLAG | H_FLAG | N_FLAG);
    CPU_SetFlag(c, Z_FLAG, (data = ((data & 0x0F) << 4) | ((data & 0xF0) >> 4)) == 0);
    Mem_WriteByte(c->memory, addr, data);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(4));
}

void SRL_Reg8(CPU *c, BYTE *reg){
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, ((*reg) & 0x01) == 1);
    CPU_SetFlag(c, Z_FLAG, ((*reg) >>= 1) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void SRL_Mem8(CPU *c, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, (data & 0x01) == 1);
    CPU_SetFlag(c, Z_FLAG, (data >>= 1) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(4));
}

// Bit Operations (CB Prefixed)
void BIT_Reg8(CPU *c, int bit, BYTE *reg){
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, true);
    CPU_SetFlag(c, Z_FLAG, ((*reg) & (0x01 << bit)) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void BIT_Mem8(CPU *c, int bit, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, true);
    CPU_SetFlag(c, Z_FLAG, (data & (0x01 << bit)) == 0);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(3));
}

void SET_Reg8(CPU *c, int bit, BYTE *reg){
    (*reg) |= (0x01 << bit);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void SET_Mem8(CPU *c, int bit, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    data |= 0x01 << bit;
    Mem_WriteByte(c->memory, addr, data);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(4));
}

void RES_Reg8(CPU *c, int bit, BYTE *reg){
    (*reg) &= ~(0x01 << bit);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(2));
}

void RES_Mem8(CPU *c, int bit, WORD addr){
    BYTE data = Mem_ReadByte(c->memory, addr);
    data &= ~(0x01 << bit);
    Mem_WriteByte(c->memory, addr, data);
    c->pc += 2;
    CPU_SetCycles(c, CYCLES(4));
}

// Misc.
void DAA(CPU *c){
    // Convert A to packed BCD
    bool NF = CPU_CheckFlag(c, N_FLAG);
    bool HF = CPU_CheckFlag(c, H_FLAG);
    bool CF = CPU_CheckFlag(c, C_FLAG);
    if(!NF && (HF || c->af.hi > 0x9)){
        c->af.hi += 0x06;
        if(CF || c->af.hi > 0x99){
            c->af.hi += 0x60;
            CPU_SetFlag(c, C_FLAG, true);
        }
        else{
            CPU_ClearFlag(c, C_FLAG);
        }
    }
    else if(NF){
        if(CF){
            c->af.hi += ((HF) ? 0x9A : 0xA0);
            CPU_SetFlag(c, C_FLAG, true);
        }
        else if(HF){
            c->af.hi += 0xFA;
            CPU_ClearFlag(c, C_FLAG);
        }
    }
    else{
        CPU_ClearFlag(c, C_FLAG);
    }
    CPU_ClearFlag(c, H_FLAG);
    CPU_SetFlag(c, Z_FLAG, c->af.hi == 0);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}

void CPL(CPU *c){
    // Complement A
    CPU_SetFlag(c, H_FLAG | N_FLAG, true);
    c->af.hi = ~(c->af.hi);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}
void CCF(CPU *c){
    // Complement carry flag
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, !CPU_CheckFlag(c, C_FLAG));
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}
void SCF(CPU *c){
    // Set carry flag
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, true);
    c->pc++;
    CPU_SetCycles(c, CYCLES(1));
}