#include "disassemble.h"

#ifdef DEBUG

static const char *instructions[] = 
{
    "NOP", "LD BC,nn", "LD (BC),A", "INC BC", "INC B", "DEC B", "LD B,n", "RLCA", "LD (nn), SP", "ADD HL,BC", "LD A,(BC)", "DEC BC", "INC C", "DEC C", "LD C,n", "RRCA",
    "STOP", "LD DE,nn", "LD (DE),A", "INC DE", "INC D", "DEC D", "LD D,n", "RLA", "JR n", "ADD HL,DE", "LD A,(DE)", "DEC DE", "INC E", "DEC E", "LD E,n", "RRA",
    "JR NZ,n", "LD HL,nn", "LDI (HL),A", "INC HL", "INC H", "DEC H", "LD H,n", "DAA", "JR Z,n", "ADD HL,HL", "LDI A,(HL)", "DEC HL", "INC L", "DEC L", "LD L,n", "CPL",
    "JR NC,n", "LD SP, nn", "LDD (HL), A", "INC SP", "INC (HL)", "DEC (HL)", "LD (HL), n", "SCF", "JR C, n", "ADD HL, SP", "LDD A, (HL)", "DEC SP", "INC A", "DEC A", "LD A, n", "CCF",
    "LD B, B", "LD B, C", "LD B, D", "LD B, E", "LD B, H", "LD B, L", "LD B, (HL)", "LD B, A", "LD C, B", "LD C, C", "LD C, D", "LD C, E", "LD C, H", "LD C, L", "LD C, (HL)", "LD C, A",
    "LD D, B", "LD D, C", "LD D, D", "LD D, E", "LD D, H", "LD D, L", "LD D, (HL)", "LD D, A", "LD E, B", "LD E, C", "LD E, D", "LD E, E", "LD E, H", "LD E, L", "LD E, (HL)", "LD E, A",
    "LD H, B", "LD H, C", "LD H, D", "LD H, E", "LD H, H", "LD H, L", "LD H, (HL)", "LD H, A", "LD L, B", "LD L, C", "LD L, D", "LD L, E", "LD L, H", "LD L, L", "LD L, (HL)", "LD L, A",
    "LD (HL), B", "LD (HL), C", "LD (HL), D", "LD (HL), E", "LD (HL), H", "LD (HL), L", "HALT", "LD (HL), A", "LD A, B", "LD A, C", "LD A, D", "LD A, E", "LD A, H", "LD A, L", "LD A, (HL)", "LD A, A",
    "ADD A, B", "ADD A, C", "ADD A, D", "ADD A, E", "ADD A, H", "ADD A, L", "ADD A, (HL)", "ADD A", "ADC B", "ADC C", "ADC D", "ADC E", "ADC H", "ADC L", "ADC (HL)", "ADC A",
    "SUB B", "SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB (HL)", "SUB A", "SBC B", "SBC C", "SBC D", "SBC E", "SBC H", "SBC L", "SBC (HL)", "SBC A",
    "AND B", "AND C", "AND D", "AND E", "AND H", "AND L", "AND (HL)", "AND A", "XOR B", "XOR C", "XOR D", "XOR E", "XOR H", "XOR L", "XOR (HL)", "XOR A",
    "OR B", "OR C", "OR D", "OR E", "OR H", "OR L", "OR (HL)", "OR A", "CP B", "CP C", "CP D", "CP E", "CP H", "CP L", "CP (HL)", "CP A",
    "RET NZ", "POP BC", "JP NZ, nn", "JP nn", "CALL NZ, nn", "PUSH BC", "ADD A, n", "RST 0x00", "RET Z", "RET", "JP Z, nn", "CB n", "CALL Z, nn", "CALL nn", "ADC n", "RST 0x08",
    "RET NC", "POP DE", "JP NC, nn", "UNKNOWN", "CALL NC, nn", "PUSH DE", "SUB n", "RST 0x10", "RET C", "RETI", "JP C, nn", "UNKNOWN", "CALL C, nn", "UNKNOWN", "SBC n", "RST 0x18",
    "LD (0xFF00 + n), A", "POP HL", "LD (0xFF00 + C), A", "UNKNOWN", "UNKNOWN", "PUSH HL", "AND n", "RST 0x20", "ADD SP,n", "JP HL", "LD (nn), A", "UNKNOWN", "UNKNOWN", "UNKNOWN", "XOR n", "RST 0x28",
    "LD A, (0xFF00 + n)", "POP AF", "LD A, (0xFF00 + C)", "DI", "UNKNOWN", "PUSH AF", "OR n", "RST 0x30", "LD HL, SP+n", "LD SP, HL", "LD A, (nn)", "EI", "UNKNOWN", "UNKNOWN", "CP n", "RST 0x38"
};

const char *Disassemble_Instruction(CPU *c){
    return instructions[c->ir];
}

#endif // DEBUG