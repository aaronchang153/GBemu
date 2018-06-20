#ifndef DECODE_H
#define DECODE_H

#include "common.h"
#include "cpu.h"
#include <stdio.h>

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

static inline void p_undef(CPU *c) { printf("Undefined opcode %x\n", c->ir); }

typedef void (*ALU_OP_REG)(CPU*, BYTE*);
typedef void (*ALU_OP_MEM)(CPU*, WORD);
typedef void (*ROT_OP_REG)(CPU*, BYTE*);
typedef void (*ROT_OP_MEM)(CPU*, WORD);

BYTE *deref_rTable(CPU *c, int index);
WORD *deref_rpTable(CPU *c, int index);
WORD *deref_rp2Table(CPU *c, int index);
BYTE deref_ccTable(int index);
ALU_OP_REG deref_aluRegTable(int index);
ALU_OP_MEM deref_aluMemTable(int index);
ROT_OP_REG deref_rotRegTable(int index);
ROT_OP_MEM deref_rotMemTable(int index);


void Decode_Execute(CPU *c);

void Decode_X_0(CPU *c);
void Decode_X_1(CPU *c);
void Decode_X_2(CPU *c);
void Decode_X_3(CPU *c);

void CB_Prefix(CPU *c);

#endif // DECODE_H