#include "opcode.h"

// Control
void NOP(CPU *c);
void STOP(CPU *c);
void HALT(CPU *c);
void DI(CPU *c);
void EI(CPU *c);
// Jumps
void JP(CPU *c);
void JP_cc(CPU *c, BYTE cond);
void JP_HL(CPU *c);
void JR(CPU *c);
void JR_cc(CPU *c, BYTE cond);
// Calls
void CALL(CPU *c);
void CALL_cc(CPU *c, BYTE cond);
// Returns
void RET(CPU *c);
void RET_cc(CPU *c, BYTE cond);
void RETI(CPU *c);
// Restart
void RST(CPU *c);
// 8-bit Loads
void LD_Imm8toReg8(CPU *c, BYTE *reg);
void LD_Imm8toMem8(CPU *c, WORD addr);
void LD_AtoMem8(CPU *c, WORD addr);
void LD_Mem8toA(CPU *c, WORD addr);
void LDI_toMem8(CPU *c, WORD *addr);
void LDI_toA(CPU *c, WORD *addr);
void LDD_toMem8(CPU *c, WORD *addr);
void LDD_toA(CPU *c, WORD *addr);
void LD_Reg8toReg8(CPU *c, BYTE *reg1, BYTE *reg2);
void LD_Mem8toReg8_addr(CPU *c, BYTE *reg, WORD addr);
void LD_Mem8toReg8_imm(CPU *c, BYTE *reg);
void LD_Reg8toMem8_addr(CPU *c, WORD addr, BYTE *reg);
void LD_Reg8toMem8_imm(CPU *c, BYTE *reg);
void LD_CtoA(CPU *c);
void LD_AtoC(CPU *c);
void LDH_NtoA(CPU *c);
void LDH_AtoN(CPU *c);
// 16-bit Loads
void LD_Imm16toReg16(CPU *c, WORD *reg);
void LD_SPtoMem16(CPU *c);
void LD_HLtoSP(CPU *c);
void LD_SPtoHL(CPU *c);
void PUSH(CPU *c, WORD *reg);
void POP(CPU *c, WORD *reg);
// 8-bit ALU (all of these operate on and store the result in A)
void ADDA_Reg8(CPU *c, BYTE *reg);
void ADDA_Mem8(CPU *c, WORD addr);
void ADCA_Reg8(CPU *c, BYTE *reg);
void ADCA_Mem8(CPU *c, WORD addr);
void SUB_Reg8(CPU *c, BYTE *reg);
void SUB_Mem8(CPU *c, WORD addr);
void SBC_Reg8(CPU *c, BYTE *reg);
void SBC_Mem8(CPU *c, WORD addr);
void AND_Reg8(CPU *c, BYTE *reg);
void AND_Mem8(CPU *c, WORD addr);
void XOR_Reg8(CPU *c, BYTE *reg);
void XOR_Mem8(CPU *c, WORD addr);
void OR_Reg8(CPU *c, BYTE *reg);
void OR_Mem8(CPU *c, WORD addr);
void CP_Reg8(CPU *c, BYTE *reg);
void CP_Mem8(CPU *c, WORD addr);
void INC_Reg8(CPU *c, BYTE *reg);
void DEC_Reg8(CPU *c, BYTE *reg);
// 16-bit ALU
void ADD_HL(CPU *c, WORD *reg);
void ADD_SP(CPU *c);
void INC_Reg16(CPU *c, WORD *reg);
void DEC_Reg16(CPU *c, WORD *reg);
// Rotates and shifts
void RLCA(CPU *c);
void RLA(CPU *c);
void RRCA(CPU *c);
void RRA(CPU *c);
void RLC_Reg8(CPU *c, BYTE *reg);
void RLC_Mem8(CPU *c, WORD addr);
void RRC_Reg8(CPU *c, BYTE *reg);
void RRC_Mem8(CPU *c, WORD addr);
void RL_Reg8(CPU *c, BYTE *reg);
void RL_Mem8(CPU *c, WORD addr);
void RR_Reg8(CPU *c, BYTE *reg);
void RR_Mem8(CPU *c, WORD addr);
void SLA_Reg8(CPU *c, BYTE *reg);
void SLA_Mem8(CPU *c, WORD addr);
void SRA_Reg8(CPU *c, BYTE *reg);
void SRA_Mem8(CPU *c, WORD addr);
void SWAP_Reg8(CPU *c, BYTE *reg);
void SWAP_Mem8(CPU *c, WORD addr);
void SRL_Reg8(CPU *c, BYTE *reg);
void SRL_Mem8(CPU *c, WORD addr);
// Bit Operations
void BIT_Reg8(CPU *c, int bit, BYTE *reg);
void BIT_Mem8(CPU *c, int bit, WORD addr);
void SET_Reg8(CPU *c, int bit, BYTE *reg);
void SET_Mem8(CPU *c, int bit, WORD addr);
void RES_Reg8(CPU *c, int bit, BYTE *reg);
void RES_Mem8(CPU *c, int bit, WORD addr);
// Misc.
void DAA(CPU *c); // Convert A to packed BCD
void CPL(CPU *c); // Complement A
void CCF(CPU *c); // Complement carry flag
void SCF(CPU *c); // Set carry flag
void CB_Prefix(CPU *c);