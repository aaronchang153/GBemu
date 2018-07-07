#include "cpu.h"
#include "decode.h"

#include <stdlib.h>
#include <stdint.h>

#define CYCLES(n) 4 * n


CPU *CPU_Create(){
    CPU *cpu = malloc(sizeof(CPU));
    return cpu;
}

void CPU_Startup(CPU *c){
    if(c != NULL){
        c->pc = 0x0100;
        c->sp = 0xFFFE;
        c->af.reg = 0x01B0;
        c->bc.reg = 0x0013;
        c->de.reg = 0x00D8;
        c->hl.reg = 0x014D;
        c->IME = false;
        c->halt = false;
        c->stop = false;
    }
}

void CPU_Destroy(CPU *c){
    if(c != NULL){
        free(c);
    }
}

void CPU_SetMemory(CPU *c, MEMORY *mem){
    c->memory = mem;
}

void CPU_Fetch(CPU *c){
    c->ir = Mem_ReadByte(c->memory, c->pc);
}

void CPU_DecodeExecute(CPU *c){
    Decode_Execute(c);
}

/* Implementation moved to the bottom

void CPU_EmulateCycle(CPU *c){
    c->ir = Mem_ReadByte(c->memory, c->pc);
    Decode_Execute(c);
}
*/

unsigned int CPU_GetCycles(CPU *c){
    return c->cycles;
}

void CPU_SetCycles(CPU *c, int cycles){
    c->cycles = cycles;
}


/***** Everything Below is Related to Decoding and Executing Instructions *****/


// Caplocked functions each take 1 machine cycle (i.e. 4 clock cycles)
static inline BYTE READ(CPU *c, WORD addr){
    c->cycles += CYCLES(1);
    return Mem_ReadByte(c->memory, addr);
}

static inline BYTE FETCH(CPU *c){
    c->cycles += CYCLES(1);
    return Mem_ReadByte(c->memory, c->pc++);
}

static inline void WRITE(CPU *c, WORD addr, BYTE data){
    Mem_WriteByte(c->memory, addr, data);
    c->cycles += CYCLES(1);
}

static inline void PC_WRITE(CPU *c, WORD data){
    c->pc = data;
    c->cycles += CYCLES(1);
}

static inline BYTE hi(WORD w) { return (BYTE) ((w & 0xFF00) >> 8); }
static inline BYTE lo(WORD w) { return (BYTE)  (w & 0x00FF); }

static inline bool hf_add(BYTE a, BYTE b) { return (((a & 0x0F) + (b & 0x0F)) & 0xF0) == 0x10; }
static inline bool cf_add(BYTE a, BYTE b) { return (((WORD) a + (WORD) b) & 0xFF00) == 0x0100; }

static inline bool hf_sub(BYTE a, BYTE b) { return (a & 0x0F) < (b & 0x0F); }
static inline bool cf_sub(BYTE a, BYTE b) { return a < b; }

static inline bool hf_add16(WORD a, WORD b) { return (((a & 0x0FFF) + (b & 0x0FFF)) & 0xF000) == 0x1000; }
static inline bool cf_add16(WORD a, WORD b) { return (((uint32_t) a + (uint32_t) b) & 0xF0000) == 0x10000; }

static inline WORD imm16(CPU *c){ WORD data = FETCH(c); data |= (FETCH(c) << 8); return data; }

static inline void rst(CPU *c, WORD data){
    // Kind of like CALL, but it's faster since the addresses are predetermined
    WRITE(c, c->sp-1, hi(c->pc));
    WRITE(c, c->sp-2, lo(c->pc));
    c->sp -= 2;
    PC_WRITE(c, data);
}

// 16-bit loads
static inline void ld_imm16(CPU *c, REGISTER *reg){
    reg->lo = FETCH(c);
    reg->hi = FETCH(c);
}

static inline void push(CPU *c, REGISTER *reg){
    WRITE(c, c->sp-1, reg->hi);
    WRITE(c, c->sp-2, reg->lo);
    c->sp -= 2;
    c->cycles += CYCLES(1);
}

static inline void pop(CPU *c, REGISTER *reg){
    reg->lo = READ(c, c->sp);
    reg->hi = READ(c, c->sp+1);
    c->sp += 2;
}

// 8-bit alu
static inline void add_a(CPU *c, BYTE data){
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, hf_add(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, cf_add(c->af.hi, data));
    CPU_SetFlag(c, Z_FLAG, (c->af.hi += data) == 0);
}

static inline void adc_a(CPU *c, BYTE data){
    bool cy = CPU_CheckFlag(c, C_FLAG);
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, hf_add(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, cf_add(c->af.hi, data));
    c->af.hi += data;
    if(cy){
        if(hf_add(c->af.hi, 1))
            CPU_SetFlag(c, H_FLAG, true);
        if(cf_add(c->af.hi, 1))
            CPU_SetFlag(c, C_FLAG, true);
        c->af.hi++;
    }
    CPU_SetFlag(c, Z_FLAG, c->af.hi == 0);
}

static inline void sub_a(CPU *c, BYTE data){
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, hf_sub(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, cf_sub(c->af.hi, data));
    CPU_SetFlag(c, Z_FLAG, (c->af.hi -= data) == 0);
}

static inline void sbc_a(CPU *c, BYTE data){
    bool cy = CPU_CheckFlag(c, C_FLAG);
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, HALF_CARRY_SUB(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, CARRY_SUB(c->af.hi, data));
    c->af.hi -= data;
    if(cy){
        if(hf_sub(c->af.hi, 1))
            CPU_SetFlag(c, H_FLAG, true);
        if(cf_sub(c->af.hi, 1))
            CPU_SetFlag(c, C_FLAG, true);
        c->af.hi--;
    }
    CPU_SetFlag(c, Z_FLAG, c->af.hi == 0);
}

static inline void and_a(CPU *c, BYTE data){
    CPU_ClearFlag(c, N_FLAG | C_FLAG);
    CPU_SetFlag(c, H_FLAG, true);
    CPU_SetFlag(c, Z_FLAG, (c->af.hi &= data) == 0);
}

static inline void or_a(CPU *c, BYTE data){
    CPU_ClearFlag(c, N_FLAG | Z_FLAG | C_FLAG);
    CPU_SetFlag(c, Z_FLAG, (c->af.hi |= data) == 0);
}

static inline void xor_a(CPU *c, BYTE data){
    CPU_ClearFlag(c, N_FLAG | Z_FLAG | C_FLAG);
    CPU_SetFlag(c, Z_FLAG, (c->af.hi ^= data) == 0);
}

static inline void cp_a(CPU *c, BYTE data){
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, c->af.hi < data);
    CPU_SetFlag(c, Z_FLAG, c->af.hi == data);
    CPU_SetFlag(c, C_FLAG, c->af.hi > data);
}

static inline void inc_r8(CPU *c, BYTE *r){
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, hf_add(*r, 1));
    CPU_SetFlag(c, Z_FLAG, (++(*r)) == 0);
}

static inline void dec_r8(CPU *c, BYTE *r){
    CPU_SetFlag(c, N_FLAG, true);
    CPU_SetFlag(c, H_FLAG, hf_sub(*r, 1));
    CPU_SetFlag(c, Z_FLAG, (--(*r)) == 0);
}

// 16-bit alu
static inline void add_hl_r16(CPU *c, WORD data){
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, hf_add16(c->hl.reg, data));
    CPU_SetFlag(c, C_FLAG, cf_add16(c->hl.reg, data));
    c->hl.reg += data;
    // Takes an extra machine cycle beyond just the fetch for some reason
    c->cycles += CYCLES(1);
}

static inline void inc_r16(CPU *c, WORD *r){
    (*r)++;
    // This also takes an extra machine cycle
    c->cycles += CYCLES(1);
}

static inline void dec_r16(CPU *c, WORD *r){
    (*r)--;
    // This one too
    c->cycles += CYCLES(1);
}

// Misc.
static inline void swap_r8(CPU *c, BYTE *r){
    CPU_ClearFlag(c, N_FLAG | H_FLAG | C_FLAG);
    BYTE temp = ((*r) & 0xF0) >> 4;
    (*r) = ((*r) << 4) | temp;
    CPU_SetFlag(c, Z_FLAG, (*r) == 0);
    // Takes an extra machine cycle
    c->cycles += CYCLES(1);
}

// Jumps
static inline void jp_nn(CPU *c){
    PC_WRITE(c, imm16(c));
}

static inline void jr_e(CPU *c){
    WORD e = FETCH(c);
    e = (e ^ 0x80) - 0x80; // sign-extend e
    PC_WRITE(c, c->pc + e);
}

// Call
static inline void call_nn(CPU *c){
    WRITE(c, c->sp-1, hi(c->pc));
    WRITE(c, c->sp-2, lo(c->pc));
    PC_WRITE(c, imm16(c));
    c->sp -= 2;
}

// Returns
static inline void ret(CPU *c){
    WORD temp = READ(c, c->sp);
    temp |= (READ(c, c->sp+1) << 8);
    PC_WRITE(c, temp);
    c->sp += 2;
}

void CPU_EmulateCycle(CPU *c){
    BYTE temp8; // temporary variable that's used by some instructions
    c->cycles = 0;
    c->ir = FETCH(c);
    switch(c->ir){
        case 0x00: // NOP
            break;
        case 0x01:
            ld_imm16(c, &c->bc);
            break;
        case 0x02:
            WRITE(c, c->bc.reg, c->af.hi);
            break;
        case 0x03:
            inc_r16(c, &c->bc);
            break;
        case 0x04:
            inc_r8(c, &c->bc.hi);
            break;
        case 0x05:
            dec_r8(c, &c->bc.hi);
            break;
        case 0x06:
            c->bc.hi = FETCH(c);
            break;
        case 0x07: // RCLA
            CPU_ClearFlag(c, H_FLAG | N_FLAG | Z_FLAG);
            CPU_SetFlag(c, C_FLAG, (temp8 = c->af.hi >> 7) == 1);
            c->af.hi = (c->af.hi << 1) | temp8;
            break;
        case 0x08:
            WRITE(c, imm16(c), c->sp);
            break;
        case 0x09:
            add_hl_r16(c, c->bc.reg);
            break;
        case 0x0A:
            c->af.hi = READ(c, c->bc.reg);
            break;
        case 0x0B:
            dec_r16(c, &c->bc.reg);
            break;
        case 0x0C:
            inc_r8(c, &c->bc.lo);
            break;
        case 0x0D:
            dec_r8(c, &c->bc.lo);
            break;
        case 0x0E:
            c->bc.lo = READ(c, FETCH(c));
            break;
        case 0x0F: // RRCA
            CPU_ClearFlag(c, H_FLAG | N_FLAG | Z_FLAG);
            CPU_SetFlag(c, C_FLAG, (temp8 = c->af.hi & 0x01) == 1);
            c->af.hi = (c->af.hi >> 1) | (temp8 << 7);
            break;

        case 0x10: // STOP
            /***************************************/
            /***** NOT ACTUALLY IMPLEMETED YET *****/
            /***************************************/
            break;
        case 0x11:
            ld_imm16(c, &c->de);
            break;
        case 0x12:
            WRITE(c, c->de.reg, c->af.hi);
            break;
        case 0x13:
            inc_r16(c, &c->de.reg);
            break;
        case 0x14:
            inc_r8(c, &c->de.hi);
            break;
        case 0x15:
            dec_r8(c, &c->de.hi);
            break;
        case 0x16:
            c->de.hi = FETCH(c);
            break;
        case 0x17: // RLA
            temp8 = (CPU_CheckFlag(c, C_FLAG)) ? 0x01 : 0x00;
            CPU_ClearFlag(c, H_FLAG | N_FLAG | Z_FLAG);
            CPU_SetFlag(c, C_FLAG, (c->af.hi >> 7) == 1);
            c->af.hi = (c->af.hi << 1) | temp8;
            break;
        case 0x18:
            jr_e(c);
            break;
        case 0x19:
            add_hl_r16(c, c->de.reg);
            break;
        case 0x1A:
            c->af.hi = READ(c, c->de.reg);
            break;
        case 0x1B:
            dec_r16(c, &c->de.reg);
            break;
        case 0x1C:
            inc_r8(c, &c->de.lo);
            break;
        case 0x1D:
            dec_r8(c, &c->de.lo);
            break;
        case 0x1E:
            c->de.lo = READ(c, FETCH(c));
            break;
        case 0x1F: // RRA
            temp8 = (CPU_CheckFlag(c, C_FLAG)) ? 0x80 : 0x00;
            CPU_ClearFlag(c, H_FLAG | N_FLAG | Z_FLAG);
            CPU_SetFlag(c, C_FLAG, (c->af.hi & 0x01) == 1);
            c->af.hi = (c->af.hi >> 1) | temp8;
            break;

        case 0x20: // JR NZ,r8
            if(!CPU_CheckFlag(c, Z_FLAG))
                jr_e(c);
            else
                PC_WRITE(c, c->pc + 1);
            break;
        case 0x21:
            ld_imm16(c, &c->hl);
            break;
        case 0x22: // LDI (HL),A
            WRITE(c, c->hl.reg, c->af.hi++);
            break;
        case 0x23:
            inc_r16(c, &c->hl.reg);
            break;
        case 0x24:
            inc_r8(c, &c->hl.hi);
            break;
        case 0x25:
            dec_r8(c, &c->hl.hi);
            break;
        case 0x26:
            c->hl.hi = FETCH(c);
            break;
        case 0x27: // DAA
            /*******************************/
            /***** NOT YET IMPLEMENTED *****/
            /*******************************/
            break;
        case 0x28: // JR Z,r8
            if(CPU_CheckFlag(c, Z_FLAG))
                jr_e(c);
            else
                PC_WRITE(c, c->pc + 1);
            break;
        case 0x29:
            add_hl_r16(c, c->hl.reg);
            break;
        case 0x2A: // LDI A,(HL)
            c->af.hi = READ(c, c->hl.reg++);
            break;
        case 0x2B:
            dec_r16(c, &c->hl.reg);
            break;
        case 0x2C:
            inc_r8(c, &c->hl.lo);
            break;
        case 0x2D:
            dec_r8(c, &c->hl.lo);
            break;
        case 0x2E:
            c->hl.lo = FETCH(c);
            break;
        case 0x2F: // CPL
            CPU_SetFlag(c, H_FLAG | N_FLAG, true);
            c->af.hi = ~(c->af.hi);
            break;

        case 0x30: // JR NC,r8;
            if(!CPU_CheckFlag(c, C_FLAG))
                jr_e(c);
            else
                PC_WRITE(c, c->pc + 1);
            break;
        case 0x31:
            c->sp = imm16(c);
            break;
        case 0x32: // LDD (HL),A
            c->af.hi = READ(c, c->hl.reg--);
            break;
        case 0x33:
            inc_r16(c, &c->sp);
            break;
        case 0x34: // INC (HL)
            CPU_ClearFlag(c, N_FLAG);
            temp8 = READ(c, c->hl.reg);
            CPU_SetFlag(c, H_FLAG, hf_add(temp8, 1));
            CPU_SetFlag(c, Z_FLAG, (++temp8) == 0);
            WRITE(c, c->hl.reg, temp8);
            break;
        case 0x35: // DEC (HL)
            CPU_SetFlag(c, N_FLAG, true);
            temp8 = READ(c, c->hl.reg);
            CPU_SetFlag(c, H_FLAG, hf_sub(temp8, 1));
            CPU_SetFlag(c, Z_FLAG, (--temp8) == 0);
            WRITE(c, c->hl.reg, temp8);
            break;
        case 0x36:
            WRITE(c, c->hl.reg, FETCH(c));
            break;
        case 0x37: // SCF
            CPU_ClearFlag(c, H_FLAG | N_FLAG);
            CPU_SetFlag(c, C_FLAG, true);
            break;
        case 0x38: // JR C,r8
            if(CPU_CheckFlag(c, C_FLAG))
                jr_e(c);
            else
                PC_WRITE(c, c->pc + 1);
            break;
        case 0x39:
            add_hl_r16(c, c->sp);
            break;
        case 0x3A: // LDD A,(HL)
            c->af.hi = READ(c, c->hl.reg--);
            break;
        case 0x3B:
            dec_r16(c, &c->sp);
            break;
        case 0x3C:
            inc_r8(c, &c->af.hi);
            break;
        case 0x3D:
            dec_r8(c, &c->af.hi);
            break;
        case 0x3E:
            c->af.hi = FETCH(c);
            break;
        case 0x3F: // CCF
            CPU_ClearFlag(c, H_FLAG | N_FLAG);
            CPU_SetFlag(c, C_FLAG, !CPU_CheckFlag(c, C_FLAG));
            break;

        case 0x40: c->bc.hi = c->bc.hi; break;
        case 0x41: c->bc.hi = c->bc.lo; break;
        case 0x42: c->bc.hi = c->de.hi; break;
        case 0x43: c->bc.hi = c->de.lo; break;
        case 0x44: c->bc.hi = c->hl.hi; break;
        case 0x45: c->bc.hi = c->hl.lo; break;
        case 0x46: c->bc.hi = READ(c, c->hl.reg); break;
        case 0x47: c->bc.hi = c->af.hi; break;

        case 0x48: c->bc.lo = c->bc.hi; break;
        case 0x49: c->bc.lo = c->bc.lo; break;
        case 0x4A: c->bc.lo = c->de.hi; break;
        case 0x4B: c->bc.lo = c->de.lo; break;
        case 0x4C: c->bc.lo = c->hl.hi; break;
        case 0x4D: c->bc.lo = c->hl.lo; break;
        case 0x4E: c->bc.lo = READ(c, c->hl.reg); break;
        case 0x4F: c->bc.lo = c->af.hi; break;

        case 0x50: c->de.hi = c->bc.hi; break;
        case 0x51: c->de.hi = c->bc.lo; break;
        case 0x52: c->de.hi = c->de.hi; break;
        case 0x53: c->de.hi = c->de.lo; break;
        case 0x54: c->de.hi = c->hl.hi; break;
        case 0x55: c->de.hi = c->hl.lo; break;
        case 0x56: c->de.hi = READ(c, c->hl.reg); break;
        case 0x57: c->de.hi = c->af.hi; break;

        case 0x58: c->de.lo = c->bc.hi; break;
        case 0x59: c->de.lo = c->bc.lo; break;
        case 0x5A: c->de.lo = c->de.hi; break;
        case 0x5B: c->de.lo = c->de.lo; break;
        case 0x5C: c->de.lo = c->hl.hi; break;
        case 0x5D: c->de.lo = c->hl.lo; break;
        case 0x5E: c->de.lo = READ(c, c->hl.reg); break;
        case 0x5F: c->de.lo = c->af.hi; break;

        case 0x60: c->hl.hi = c->bc.hi; break;
        case 0x61: c->hl.hi = c->bc.lo; break;
        case 0x62: c->hl.hi = c->de.hi; break;
        case 0x63: c->hl.hi = c->de.lo; break;
        case 0x64: c->hl.hi = c->hl.hi; break;
        case 0x65: c->hl.hi = c->hl.lo; break;
        case 0x66: c->hl.hi = READ(c, c->hl.reg); break;
        case 0x67: c->hl.hi = c->af.hi; break;

        case 0x68: c->hl.lo = c->bc.hi; break;
        case 0x69: c->hl.lo = c->bc.lo; break;
        case 0x6A: c->hl.lo = c->de.hi; break;
        case 0x6B: c->hl.lo = c->de.lo; break;
        case 0x6C: c->hl.lo = c->hl.hi; break;
        case 0x6D: c->hl.lo = c->hl.lo; break;
        case 0x6E: c->hl.lo = READ(c, c->hl.reg); break;
        case 0x6F: c->hl.lo = c->af.hi; break;

        case 0x70: WRITE(c, c->hl.reg, c->bc.hi); break;
        case 0x71: WRITE(c, c->hl.reg, c->bc.lo); break;
        case 0x72: WRITE(c, c->hl.reg, c->de.hi); break;
        case 0x73: WRITE(c, c->hl.reg, c->de.lo); break;
        case 0x74: WRITE(c, c->hl.reg, c->hl.hi); break;
        case 0x75: WRITE(c, c->hl.reg, c->hl.lo); break;
        case 0x76: c->halt = true; break; // HALT replaces LD (HL),(HL)
        case 0x77: WRITE(c, c->hl.reg, c->af.hi); break;

        case 0x78: c->af.hi = c->bc.hi; break;
        case 0x79: c->af.hi = c->bc.lo; break;
        case 0x7A: c->af.hi = c->de.hi; break;
        case 0x7B: c->af.hi = c->de.lo; break;
        case 0x7C: c->af.hi = c->hl.hi; break;
        case 0x7D: c->af.hi = c->hl.lo; break;
        case 0x7E: c->af.hi = READ(c, c->hl.reg); break;
        case 0x7F: c->af.hi = c->af.hi; break;

        case 0x80: add_a(c, c->bc.hi); break;
        case 0x81: add_a(c, c->bc.lo); break;
        case 0x82: add_a(c, c->de.hi); break;
        case 0x83: add_a(c, c->de.lo); break;
        case 0x84: add_a(c, c->hl.hi); break;
        case 0x85: add_a(c, c->hl.lo); break;
        case 0x86: add_a(c, READ(c, c->hl.reg)); break;
        case 0x87: add_a(c, c->af.hi); break;

        case 0x88: adc_a(c, c->bc.hi); break;
        case 0x89: adc_a(c, c->bc.lo); break;
        case 0x8A: adc_a(c, c->de.hi); break;
        case 0x8B: adc_a(c, c->de.lo); break;
        case 0x8C: adc_a(c, c->hl.hi); break;
        case 0x8D: adc_a(c, c->hl.lo); break;
        case 0x8E: adc_a(c, READ(c, c->hl.reg)); break;
        case 0x8F: adc_a(c, c->af.hi); break;

        case 0x90: sub_a(c, c->bc.hi); break;
        case 0x91: sub_a(c, c->bc.lo); break;
        case 0x92: sub_a(c, c->de.hi); break;
        case 0x93: sub_a(c, c->de.lo); break;
        case 0x94: sub_a(c, c->hl.hi); break;
        case 0x95: sub_a(c, c->hl.lo); break;
        case 0x96: sub_a(c, READ(c, c->hl.reg)); break;
        case 0x97: sub_a(c, c->af.hi); break;

        case 0x98: sbc_a(c, c->bc.hi); break;
        case 0x99: sbc_a(c, c->bc.lo); break;
        case 0x9A: sbc_a(c, c->de.hi); break;
        case 0x9B: sbc_a(c, c->de.lo); break;
        case 0x9C: sbc_a(c, c->hl.hi); break;
        case 0x9D: sbc_a(c, c->hl.lo); break;
        case 0x9E: sbc_a(c, READ(c, c->hl.reg)); break;
        case 0x9F: sbc_a(c, c->af.hi); break;

        case 0xA0: and_a(c, c->bc.hi); break;
        case 0xA1: and_a(c, c->bc.lo); break;
        case 0xA2: and_a(c, c->de.hi); break;
        case 0xA3: and_a(c, c->de.lo); break;
        case 0xA4: and_a(c, c->hl.hi); break;
        case 0xA5: and_a(c, c->hl.lo); break;
        case 0xA6: and_a(c, READ(c, c->hl.reg)); break;
        case 0xA7: and_a(c, c->af.hi); break;

        case 0xA8: xor_a(c, c->bc.hi); break;
        case 0xA9: xor_a(c, c->bc.lo); break;
        case 0xAA: xor_a(c, c->de.hi); break;
        case 0xAB: xor_a(c, c->de.lo); break;
        case 0xAC: xor_a(c, c->hl.hi); break;
        case 0xAD: xor_a(c, c->hl.lo); break;
        case 0xAE: xor_a(c, READ(c, c->hl.reg)); break;
        case 0xAF: xor_a(c, c->af.hi); break;

        case 0xB0: or_a(c, c->bc.hi); break;
        case 0xB1: or_a(c, c->bc.lo); break;
        case 0xB2: or_a(c, c->de.hi); break;
        case 0xB3: or_a(c, c->de.lo); break;
        case 0xB4: or_a(c, c->hl.hi); break;
        case 0xB5: or_a(c, c->hl.lo); break;
        case 0xB6: or_a(c, READ(c, c->hl.reg)); break;
        case 0xB7: or_a(c, c->af.hi); break;

        case 0xB8: cp_a(c, c->bc.hi); break;
        case 0xB9: cp_a(c, c->bc.lo); break;
        case 0xBA: cp_a(c, c->de.hi); break;
        case 0xBB: cp_a(c, c->de.lo); break;
        case 0xBC: cp_a(c, c->hl.hi); break;
        case 0xBD: cp_a(c, c->hl.lo); break;
        case 0xBE: cp_a(c, READ(c, c->hl.reg)); break;
        case 0xBF: cp_a(c, c->af.hi); break;

        case 0xC0: // RET NZ
            if(!CPU_CheckFlag(c, Z_FLAG)){
                ret(c);
                c->cycles += CYCLES(1);
            }
            else{
                PC_WRITE(c, c->pc + 1);
            }
            break;
        case 0xC1:
            pop(c, &c->bc);
            break;
        case 0xC2: // JP NZ,nn
            if(!CPU_CheckFlag(c, Z_FLAG)){
                jp_nn(c);
            }
            else{
                c->pc += 2; // skip over nn
                c->cycles += CYCLES(2);
            }
            break;
        case 0xC3:
            jp_nn(c);
            break;
        case 0xC4: // CALL NZ,nn
            if(!CPU_CheckFlag(c, Z_FLAG)){
                call_nn(c);
            }
            else{
                c->pc += 2;
                c->cycles += CYCLES(2);
            }
            break;
        case 0xC5:
            push(c, &c->bc);
            break;
        case 0xC6:
            add_a(c, FETCH(c));
            break;
        case 0xC7:
            rst(c, 0x00);
            break;
        case 0xC8: // RET Z
            if(CPU_CheckFlag(c, Z_FLAG))
                ret(c);
            c->cycles += CYCLES(1);
            break;
        case 0xC9:
            ret(c);
            break;
        case 0xCA: // JP Z,nn
            if(CPU_CheckFlag(c, Z_FLAG)){
                jp_nn(c);
            }
            else{
                c->pc += 2;
                c->cycles += CYCLES(2);
            }
            break;
        case 0xCB: // CB Prefix
            /***** To be implemented *****/
            break;
        case 0xCC: // CALL Z,nn
            if(CPU_CheckFlag(c, Z_FLAG)){
                call_nn(c);
            }
            else{
                c->pc += 2;
                c->cycles += CYCLES(2);
            }
            break;
        case 0xCD:
            call_nn(c);
            break;
        case 0xCE:
            adc_a(c, FETCH(c));
            break;
        case 0xCF:
            rst(c, 0x08);
            break;
    };
}