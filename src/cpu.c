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
    //Decode_Execute(c);
    CPU_EmulateCycle(c);
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
    CPU_SetFlag(c, H_FLAG, hf_sub(c->af.hi, data));
    CPU_SetFlag(c, C_FLAG, cf_sub(c->af.hi, data));
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
static inline void swap(CPU *c, BYTE *r){
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
    WRITE(c, c->sp-1, hi(c->pc + 2));
    WRITE(c, c->sp-2, lo(c->pc + 2));
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

// CB Prefixed Rotates and Shifts
static inline void rlc(CPU *c, BYTE *r){
    BYTE rotate;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, (rotate = (*r) >> 7) == 1);
    CPU_SetFlag(c, Z_FLAG, (*r = ((*r) << 1) | rotate) == 0);
}

static inline void rl(CPU *c, BYTE *r){
    BYTE rotate = (CPU_CheckFlag(c, C_FLAG)) ? 0x01 : 0x00;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, ((*r) >> 7) == 1);
    CPU_SetFlag(c, Z_FLAG, (*r = ((*r) << 1) | rotate) == 0);
}

static inline void rrc(CPU *c, BYTE *r){
    BYTE rotate;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, (rotate = (*r) & 0x01) == 1);
    CPU_SetFlag(c, Z_FLAG, (*r = ((*r) >> 1) | (rotate << 7)) == 0);
}

static inline void rr(CPU *c, BYTE *r){
    BYTE rotate = (CPU_CheckFlag(c, C_FLAG)) ? 0x80 : 0x00;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, ((*r) & 0x01) == 1);
    CPU_SetFlag(c, Z_FLAG, (*r = ((*r) >> 1) | rotate) == 0);
}

static inline void sla(CPU *c, BYTE *r){
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, ((*r) & 0x80) == 0x80);
    CPU_SetFlag(c, Z_FLAG, ((*r) <<= 1) == 0);
}

static inline void sra(CPU *c, BYTE *r){
    BYTE extend = (*r) & 0x80;
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, ((*r) & 0x01) == 0x01);
    CPU_SetFlag(c, Z_FLAG, (*r = ((*r) >> 1) | extend) == 0);
}

static inline void srl(CPU *c, BYTE *r){
    CPU_ClearFlag(c, H_FLAG | N_FLAG);
    CPU_SetFlag(c, C_FLAG, ((*r) & 0x01) == 1);
    CPU_SetFlag(c, Z_FLAG, ((*r) >>= 1) == 0);
}

// CB Prefixed Bit Operations
static inline void bit(CPU *c, BYTE *r, int b){
    CPU_ClearFlag(c, N_FLAG);
    CPU_SetFlag(c, H_FLAG, true);
    CPU_SetFlag(c, Z_FLAG, !TEST_BIT(*r, b));
    c->cycles += CYCLES(1);
}

static inline void set(CPU *c, BYTE *r, int b){
    (*r) |= (0x01 << b);
}

static inline void res(CPU *c, BYTE *r, int b){
    (*r) &= ~(0x01 << b);
}


void CPU_EmulateCycle(CPU *c){
    BYTE temp8; // temporary variable that's used by some instructions
    WORD temp16;
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
            inc_r16(c, &c->bc.reg);
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
            c->bc.lo = FETCH(c);
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
            WRITE(c, c->hl.reg++, c->af.hi);
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
            temp16 = c->af.hi;
            if(CPU_CheckFlag(c, N_FLAG)) {
                if(CPU_CheckFlag(c, H_FLAG))
                    temp16 += 0xFA;
                if(CPU_CheckFlag(c, C_FLAG))
                    temp16 -= 0x60;
            }
            else {
                if(CPU_CheckFlag(c, H_FLAG) || (temp16 & 0xF) > 9)
                    temp16 += 0x06;
                if(CPU_CheckFlag(c, C_FLAG) || temp16 > 0x9F)
                    temp16 += 0x60;
            }
            c->af.hi = temp16 & 0xFF;
            CPU_ClearFlag(c, H_FLAG);
            CPU_SetFlag(c, C_FLAG, temp16 > 0xFF);
            CPU_SetFlag(c, Z_FLAG, c->af.hi == 0);
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
            if(!CPU_CheckFlag(c, Z_FLAG))
                ret(c);
            c->cycles += CYCLES(1);
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
            c->ir = FETCH(c);
            switch(c->ir){
                case 0x00: rlc(c, &c->bc.hi); break;
                case 0x01: rlc(c, &c->bc.lo); break;
                case 0x02: rlc(c, &c->de.hi); break;
                case 0x03: rlc(c, &c->de.lo); break;
                case 0x04: rlc(c, &c->hl.hi); break;
                case 0x05: rlc(c, &c->hl.lo); break;
                case 0x06: temp8 = READ(c, c->hl.reg); rlc(c, &temp8); WRITE(c, c->hl.reg, temp8); break;
                case 0x07: rrc(c, &c->af.hi); break;
                case 0x08: rrc(c, &c->bc.hi); break;
                case 0x09: rrc(c, &c->bc.lo); break;
                case 0x0A: rrc(c, &c->de.hi); break;
                case 0x0B: rrc(c, &c->de.lo); break;
                case 0x0C: rrc(c, &c->hl.hi); break;
                case 0x0D: rrc(c, &c->hl.lo); break;
                case 0x0E: temp8 = READ(c, c->hl.reg); rrc(c, &temp8); WRITE(c, c->hl.reg, temp8); break;
                case 0x0F: rrc(c, &c->af.hi); break;

                case 0x10: rl(c, &c->bc.hi); break;
                case 0x11: rl(c, &c->bc.lo); break;
                case 0x12: rl(c, &c->de.hi); break;
                case 0x13: rl(c, &c->de.lo); break;
                case 0x14: rl(c, &c->hl.hi); break;
                case 0x15: rl(c, &c->hl.lo); break;
                case 0x16: temp8 = READ(c, c->hl.reg); rl(c, &temp8); WRITE(c, c->hl.reg, temp8); break;
                case 0x17: rl(c, &c->af.hi); break;
                case 0x18: rr(c, &c->bc.hi); break;
                case 0x19: rr(c, &c->bc.lo); break;
                case 0x1A: rr(c, &c->de.hi); break;
                case 0x1B: rr(c, &c->de.lo); break;
                case 0x1C: rr(c, &c->hl.hi); break;
                case 0x1D: rr(c, &c->hl.lo); break;
                case 0x1E: temp8 = READ(c, c->hl.reg); rr(c, &temp8); WRITE(c, c->hl.reg, temp8); break;
                case 0x1F: rr(c, &c->af.hi); break;

                case 0x20: sla(c, &c->bc.hi); break;
                case 0x21: sla(c, &c->bc.lo); break;
                case 0x22: sla(c, &c->de.hi); break;
                case 0x23: sla(c, &c->de.lo); break;
                case 0x24: sla(c, &c->hl.hi); break;
                case 0x25: sla(c, &c->hl.lo); break;
                case 0x26: temp8 = READ(c, c->hl.reg); sla(c, &temp8); WRITE(c, c->hl.reg, temp8); break;
                case 0x27: sla(c, &c->af.hi); break;
                case 0x28: sra(c, &c->bc.hi); break;
                case 0x29: sra(c, &c->bc.lo); break;
                case 0x2A: sra(c, &c->de.hi); break;
                case 0x2B: sra(c, &c->de.lo); break;
                case 0x2C: sra(c, &c->hl.hi); break;
                case 0x2D: sra(c, &c->hl.lo); break;
                case 0x2E: temp8 = READ(c, c->hl.reg); sra(c, &temp8); WRITE(c, c->hl.reg, temp8); break;
                case 0x2F: sra(c, &c->af.hi); break;

                case 0x30: swap(c, &c->bc.hi); break;
                case 0x31: swap(c, &c->bc.lo); break;
                case 0x32: swap(c, &c->de.hi); break;
                case 0x33: swap(c, &c->de.lo); break;
                case 0x34: swap(c, &c->hl.hi); break;
                case 0x35: swap(c, &c->hl.lo); break;
                case 0x36: temp8 = READ(c, c->hl.reg); swap(c, &temp8); WRITE(c, c->hl.reg, temp8); break;
                case 0x37: swap(c, &c->af.hi); break;
                case 0x38: srl(c, &c->bc.hi); break;
                case 0x39: srl(c, &c->bc.lo); break;
                case 0x3A: srl(c, &c->de.hi); break;
                case 0x3B: srl(c, &c->de.lo); break;
                case 0x3C: srl(c, &c->hl.hi); break;
                case 0x3D: srl(c, &c->hl.lo); break;
                case 0x3E: temp8 = READ(c, c->hl.reg); srl(c, &temp8); WRITE(c, c->hl.reg, temp8); break;
                case 0x3F: srl(c, &c->af.hi); break;

                case 0x40: bit(c, &c->bc.hi, 0); break;
                case 0x41: bit(c, &c->bc.lo, 0); break;
                case 0x42: bit(c, &c->de.hi, 0); break;
                case 0x43: bit(c, &c->de.lo, 0); break;
                case 0x44: bit(c, &c->hl.hi, 0); break;
                case 0x45: bit(c, &c->hl.lo, 0); break;
                case 0x46: temp8 = READ(c, c->hl.reg); bit(c, &temp8, 0); WRITE(c, c->hl.reg, temp8); break;
                case 0x47: bit(c, &c->af.hi, 0); break;
                case 0x48: bit(c, &c->bc.hi, 1); break;
                case 0x49: bit(c, &c->bc.lo, 1); break;
                case 0x4A: bit(c, &c->de.hi, 1); break;
                case 0x4B: bit(c, &c->de.lo, 1); break;
                case 0x4C: bit(c, &c->hl.hi, 1); break;
                case 0x4D: bit(c, &c->hl.lo, 1); break;
                case 0x4E: temp8 = READ(c, c->hl.reg); bit(c, &temp8, 1); WRITE(c, c->hl.reg, temp8); break;
                case 0x4F: bit(c, &c->af.hi, 1); break;

                case 0x50: bit(c, &c->bc.hi, 2); break;
                case 0x51: bit(c, &c->bc.lo, 2); break;
                case 0x52: bit(c, &c->de.hi, 2); break;
                case 0x53: bit(c, &c->de.lo, 2); break;
                case 0x54: bit(c, &c->hl.hi, 2); break;
                case 0x55: bit(c, &c->hl.lo, 2); break;
                case 0x56: temp8 = READ(c, c->hl.reg); bit(c, &temp8, 2); WRITE(c, c->hl.reg, temp8); break;
                case 0x57: bit(c, &c->af.hi, 2); break;
                case 0x58: bit(c, &c->bc.hi, 3); break;
                case 0x59: bit(c, &c->bc.lo, 3); break;
                case 0x5A: bit(c, &c->de.hi, 3); break;
                case 0x5B: bit(c, &c->de.lo, 3); break;
                case 0x5C: bit(c, &c->hl.hi, 3); break;
                case 0x5D: bit(c, &c->hl.lo, 3); break;
                case 0x5E: temp8 = READ(c, c->hl.reg); bit(c, &temp8, 3); WRITE(c, c->hl.reg, temp8); break;
                case 0x5F: bit(c, &c->af.hi, 3); break;

                case 0x60: bit(c, &c->bc.hi, 4); break;
                case 0x61: bit(c, &c->bc.lo, 4); break;
                case 0x62: bit(c, &c->de.hi, 4); break;
                case 0x63: bit(c, &c->de.lo, 4); break;
                case 0x64: bit(c, &c->hl.hi, 4); break;
                case 0x65: bit(c, &c->hl.lo, 4); break;
                case 0x66: temp8 = READ(c, c->hl.reg); bit(c, &temp8, 4); WRITE(c, c->hl.reg, temp8); break;
                case 0x67: bit(c, &c->af.hi, 4); break;
                case 0x68: bit(c, &c->bc.hi, 5); break;
                case 0x69: bit(c, &c->bc.lo, 5); break;
                case 0x6A: bit(c, &c->de.hi, 5); break;
                case 0x6B: bit(c, &c->de.lo, 5); break;
                case 0x6C: bit(c, &c->hl.hi, 5); break;
                case 0x6D: bit(c, &c->hl.lo, 5); break;
                case 0x6E: temp8 = READ(c, c->hl.reg); bit(c, &temp8, 5); WRITE(c, c->hl.reg, temp8); break;
                case 0x6F: bit(c, &c->af.hi, 5); break;

                case 0x70: bit(c, &c->bc.hi, 6); break;
                case 0x71: bit(c, &c->bc.lo, 6); break;
                case 0x72: bit(c, &c->de.hi, 6); break;
                case 0x73: bit(c, &c->de.lo, 6); break;
                case 0x74: bit(c, &c->hl.hi, 6); break;
                case 0x75: bit(c, &c->hl.lo, 6); break;
                case 0x76: temp8 = READ(c, c->hl.reg); bit(c, &temp8, 6); WRITE(c, c->hl.reg, temp8); break;
                case 0x77: bit(c, &c->af.hi, 6); break;
                case 0x78: bit(c, &c->bc.hi, 7); break;
                case 0x79: bit(c, &c->bc.lo, 7); break;
                case 0x7A: bit(c, &c->de.hi, 7); break;
                case 0x7B: bit(c, &c->de.lo, 7); break;
                case 0x7C: bit(c, &c->hl.hi, 7); break;
                case 0x7D: bit(c, &c->hl.lo, 7); break;
                case 0x7E: temp8 = READ(c, c->hl.reg); bit(c, &temp8, 7); WRITE(c, c->hl.reg, temp8); break;
                case 0x7F: bit(c, &c->af.hi, 7); break;

                case 0x80: res(c, &c->bc.hi, 0); break;
                case 0x81: res(c, &c->bc.lo, 0); break;
                case 0x82: res(c, &c->de.hi, 0); break;
                case 0x83: res(c, &c->de.lo, 0); break;
                case 0x84: res(c, &c->hl.hi, 0); break;
                case 0x85: res(c, &c->hl.lo, 0); break;
                case 0x86: temp8 = READ(c, c->hl.reg); res(c, &temp8, 0); WRITE(c, c->hl.reg, temp8); break;
                case 0x87: res(c, &c->af.hi, 0); break;
                case 0x88: res(c, &c->bc.hi, 1); break;
                case 0x89: res(c, &c->bc.lo, 1); break;
                case 0x8A: res(c, &c->de.hi, 1); break;
                case 0x8B: res(c, &c->de.lo, 1); break;
                case 0x8C: res(c, &c->hl.hi, 1); break;
                case 0x8D: res(c, &c->hl.lo, 1); break;
                case 0x8E: temp8 = READ(c, c->hl.reg); res(c, &temp8, 1); WRITE(c, c->hl.reg, temp8); break;
                case 0x8F: res(c, &c->af.hi, 1); break;

                case 0x90: res(c, &c->bc.hi, 2); break;
                case 0x91: res(c, &c->bc.lo, 2); break;
                case 0x92: res(c, &c->de.hi, 2); break;
                case 0x93: res(c, &c->de.lo, 2); break;
                case 0x94: res(c, &c->hl.hi, 2); break;
                case 0x95: res(c, &c->hl.lo, 2); break;
                case 0x96: temp8 = READ(c, c->hl.reg); res(c, &temp8, 2); WRITE(c, c->hl.reg, temp8); break;
                case 0x97: res(c, &c->af.hi, 2); break;
                case 0x98: res(c, &c->bc.hi, 3); break;
                case 0x99: res(c, &c->bc.lo, 3); break;
                case 0x9A: res(c, &c->de.hi, 3); break;
                case 0x9B: res(c, &c->de.lo, 3); break;
                case 0x9C: res(c, &c->hl.hi, 3); break;
                case 0x9D: res(c, &c->hl.lo, 3); break;
                case 0x9E: temp8 = READ(c, c->hl.reg); res(c, &temp8, 3); WRITE(c, c->hl.reg, temp8); break;
                case 0x9F: res(c, &c->af.hi, 3); break;

                case 0xA0: res(c, &c->bc.hi, 4); break;
                case 0xA1: res(c, &c->bc.lo, 4); break;
                case 0xA2: res(c, &c->de.hi, 4); break;
                case 0xA3: res(c, &c->de.lo, 4); break;
                case 0xA4: res(c, &c->hl.hi, 4); break;
                case 0xA5: res(c, &c->hl.lo, 4); break;
                case 0xA6: temp8 = READ(c, c->hl.reg); res(c, &temp8, 4); WRITE(c, c->hl.reg, temp8); break;
                case 0xA7: res(c, &c->af.hi, 4); break;
                case 0xA8: res(c, &c->bc.hi, 5); break;
                case 0xA9: res(c, &c->bc.lo, 5); break;
                case 0xAA: res(c, &c->de.hi, 5); break;
                case 0xAB: res(c, &c->de.lo, 5); break;
                case 0xAC: res(c, &c->hl.hi, 5); break;
                case 0xAD: res(c, &c->hl.lo, 5); break;
                case 0xAE: temp8 = READ(c, c->hl.reg); res(c, &temp8, 5); WRITE(c, c->hl.reg, temp8); break;
                case 0xAF: res(c, &c->af.hi, 5); break;

                case 0xB0: res(c, &c->bc.hi, 6); break;
                case 0xB1: res(c, &c->bc.lo, 6); break;
                case 0xB2: res(c, &c->de.hi, 6); break;
                case 0xB3: res(c, &c->de.lo, 6); break;
                case 0xB4: res(c, &c->hl.hi, 6); break;
                case 0xB5: res(c, &c->hl.lo, 6); break;
                case 0xB6: temp8 = READ(c, c->hl.reg); res(c, &temp8, 6); WRITE(c, c->hl.reg, temp8); break;
                case 0xB7: res(c, &c->af.hi, 6); break;
                case 0xB8: res(c, &c->bc.hi, 7); break;
                case 0xB9: res(c, &c->bc.lo, 7); break;
                case 0xBA: res(c, &c->de.hi, 7); break;
                case 0xBB: res(c, &c->de.lo, 7); break;
                case 0xBC: res(c, &c->hl.hi, 7); break;
                case 0xBD: res(c, &c->hl.lo, 7); break;
                case 0xBE: temp8 = READ(c, c->hl.reg); res(c, &temp8, 7); WRITE(c, c->hl.reg, temp8); break;
                case 0xBF: res(c, &c->af.hi, 7); break;

                case 0xC0: set(c, &c->bc.hi, 0); break;
                case 0xC1: set(c, &c->bc.lo, 0); break;
                case 0xC2: set(c, &c->de.hi, 0); break;
                case 0xC3: set(c, &c->de.lo, 0); break;
                case 0xC4: set(c, &c->hl.hi, 0); break;
                case 0xC5: set(c, &c->hl.lo, 0); break;
                case 0xC6: temp8 = READ(c, c->hl.reg); set(c, &temp8, 0); WRITE(c, c->hl.reg, temp8); break;
                case 0xC7: set(c, &c->af.hi, 0); break;
                case 0xC8: set(c, &c->bc.hi, 1); break;
                case 0xC9: set(c, &c->bc.lo, 1); break;
                case 0xCA: set(c, &c->de.hi, 1); break;
                case 0xCB: set(c, &c->de.lo, 1); break;
                case 0xCC: set(c, &c->hl.hi, 1); break;
                case 0xCD: set(c, &c->hl.lo, 1); break;
                case 0xCE: temp8 = READ(c, c->hl.reg); set(c, &temp8, 1); WRITE(c, c->hl.reg, temp8); break;
                case 0xCF: set(c, &c->af.hi, 1); break;

                case 0xD0: set(c, &c->bc.hi, 2); break;
                case 0xD1: set(c, &c->bc.lo, 2); break;
                case 0xD2: set(c, &c->de.hi, 2); break;
                case 0xD3: set(c, &c->de.lo, 2); break;
                case 0xD4: set(c, &c->hl.hi, 2); break;
                case 0xD5: set(c, &c->hl.lo, 2); break;
                case 0xD6: temp8 = READ(c, c->hl.reg); set(c, &temp8, 2); WRITE(c, c->hl.reg, temp8); break;
                case 0xD7: set(c, &c->af.hi, 2); break;
                case 0xD8: set(c, &c->bc.hi, 3); break;
                case 0xD9: set(c, &c->bc.lo, 3); break;
                case 0xDA: set(c, &c->de.hi, 3); break;
                case 0xDB: set(c, &c->de.lo, 3); break;
                case 0xDC: set(c, &c->hl.hi, 3); break;
                case 0xDD: set(c, &c->hl.lo, 3); break;
                case 0xDE: temp8 = READ(c, c->hl.reg); set(c, &temp8, 3); WRITE(c, c->hl.reg, temp8); break;
                case 0xDF: set(c, &c->af.hi, 3); break;

                case 0xE0: set(c, &c->bc.hi, 4); break;
                case 0xE1: set(c, &c->bc.lo, 4); break;
                case 0xE2: set(c, &c->de.hi, 4); break;
                case 0xE3: set(c, &c->de.lo, 4); break;
                case 0xE4: set(c, &c->hl.hi, 4); break;
                case 0xE5: set(c, &c->hl.lo, 4); break;
                case 0xE6: temp8 = READ(c, c->hl.reg); set(c, &temp8, 4); WRITE(c, c->hl.reg, temp8); break;
                case 0xE7: set(c, &c->af.hi, 4); break;
                case 0xE8: set(c, &c->bc.hi, 5); break;
                case 0xE9: set(c, &c->bc.lo, 5); break;
                case 0xEA: set(c, &c->de.hi, 5); break;
                case 0xEB: set(c, &c->de.lo, 5); break;
                case 0xEC: set(c, &c->hl.hi, 5); break;
                case 0xED: set(c, &c->hl.lo, 5); break;
                case 0xEE: temp8 = READ(c, c->hl.reg); set(c, &temp8, 5); WRITE(c, c->hl.reg, temp8); break;
                case 0xEF: set(c, &c->af.hi, 5); break;

                case 0xF0: set(c, &c->bc.hi, 6); break;
                case 0xF1: set(c, &c->bc.lo, 6); break;
                case 0xF2: set(c, &c->de.hi, 6); break;
                case 0xF3: set(c, &c->de.lo, 6); break;
                case 0xF4: set(c, &c->hl.hi, 6); break;
                case 0xF5: set(c, &c->hl.lo, 6); break;
                case 0xF6: temp8 = READ(c, c->hl.reg); set(c, &temp8, 6); WRITE(c, c->hl.reg, temp8); break;
                case 0xF7: set(c, &c->af.hi, 6); break;
                case 0xF8: set(c, &c->bc.hi, 7); break;
                case 0xF9: set(c, &c->bc.lo, 7); break;
                case 0xFA: set(c, &c->de.hi, 7); break;
                case 0xFB: set(c, &c->de.lo, 7); break;
                case 0xFC: set(c, &c->hl.hi, 7); break;
                case 0xFD: set(c, &c->hl.lo, 7); break;
                case 0xFE: temp8 = READ(c, c->hl.reg); set(c, &temp8, 7); WRITE(c, c->hl.reg, temp8); break;
                case 0xFF: set(c, &c->af.hi, 7); break;
            };
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

        case 0xD0: // RET NC
            if(!CPU_CheckFlag(c, C_FLAG))
                ret(c);
            c->cycles += CYCLES(1);
            break;
        case 0xD1:
            pop(c, &c->de);
            break;
        case 0xD2: // JP NC,nn
            if(!CPU_CheckFlag(c, C_FLAG)){
                jp_nn(c);
            }
            else{
                c->pc += 2; // skip over nn
                c->cycles += CYCLES(2);
            }
            break;
        case 0xD3: // Unused
            break;
        case 0xD4: // CALL NZ,nn
            if(!CPU_CheckFlag(c, C_FLAG)){
                call_nn(c);
            }
            else{
                c->pc += 2;
                c->cycles += CYCLES(2);
            }
            break;
        case 0xD5:
            push(c, &c->de);
            break;
        case 0xD6:
            sub_a(c, FETCH(c));
            break;
        case 0xD7:
            rst(c, 0x10);
            break;
        case 0xD8: // RET C
            if(CPU_CheckFlag(c, C_FLAG))
                ret(c);
            c->cycles += CYCLES(1);
            break;
        case 0xD9: // RETI
            ret(c);
            c->IME = true;
            break;
        case 0xDA: // JP C,nn
            if(CPU_CheckFlag(c, C_FLAG)){
                jp_nn(c);
            }
            else{
                c->pc += 2;
                c->cycles += CYCLES(2);
            }
            break;
        case 0xDB: // Unused
            break;
        case 0xDC: // CALL C,nn
            if(CPU_CheckFlag(c, C_FLAG)){
                call_nn(c);
            }
            else{
                c->pc += 2;
                c->cycles += CYCLES(2);
            }
            break;
        case 0xDD: // Unused
            break;
        case 0xDE:
            sbc_a(c, FETCH(c));
            break;
        case 0xDF:
            rst(c, 0x18);
            break;

        case 0xE0: // LDH ($FF00 + n),A
            WRITE(c, 0xFF00 + FETCH(c), c->af.hi);
            break;
        case 0xE1:
            pop(c, &c->hl);
            break;
        case 0xE2: // LD ($FF00 + C), A
            WRITE(c, 0xFF00 + c->bc.lo, c->af.hi);
            break;
        case 0xE3: // Unused
        case 0xE4: // Unused
            break;
        case 0xE5:
            push(c, &c->hl);
            break;
        case 0xE6:
            and_a(c, FETCH(c));
            break;
        case 0xE7:
            rst(c, 0x20);
            break;
        case 0xE8: // ADD SP,e
            temp16 = FETCH(c);
            temp16 = (temp16 ^ 0x80) - 0x80; // sign-extend
            c->sp += temp16;
            c->cycles += CYCLES(2);
            break;
        case 0xE9:
            c->pc = c->hl.reg;
            break;
        case 0xEA:
            WRITE(c, imm16(c), c->af.hi);
            break;
        case 0xEB: // Unused
        case 0xEC: // Unused
        case 0xED: // Unused
            break;
        case 0xEE:
            xor_a(c, FETCH(c));
            break;
        case 0xEF:
            rst(c, 0x28);
            break;
        case 0xF0: // LDH A,($FF00 + n)
            c->af.hi = READ(c, 0xFF00 + FETCH(c));
            break;
        case 0xF1:
            pop(c, &c->af);
            break;
        case 0xF2: // LD A,($FF00 + C)
            c->af.hi = READ(c, 0xFF00 + c->bc.lo);
            break;
        case 0xF3: // DI
            c->IME = false;
            break;
        case 0xF4: // Unused
            break;
        case 0xF5:
            push(c, &c->af);
            break;
        case 0xF6:
            or_a(c, FETCH(c));
            break;
        case 0xF7:
            rst(c, 0x30);
            break;
        case 0xF8: // LDHL SP+e
            temp16 = FETCH(c);
            temp16 = (temp16 ^ 0x08) - 0x80; // sign-extend
            c->hl.reg = c->sp + temp16;
            c->cycles += CYCLES(1);
            break;
        case 0xF9:
            c->sp = c->hl.reg;
            c->cycles += CYCLES(1);
            break;
        case 0xFA:
            c->af.hi = READ(c, imm16(c));
            break;
        case 0xFB: // EI
            c->IME = true;
            break;
        case 0xFC: // Unused
        case 0xFD: // Unused
            break;
        case 0xFE:
            cp_a(c, FETCH(c));
            break;
        case 0xFF:
            rst(c, 0x38);
            break;
    };
}
