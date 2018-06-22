#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H

#ifdef DEBUG

#include "cpu.h"

const char *Disassemble_Instruction(CPU *c);

#endif // DEBUG

#endif // DISASSEMBLE_H