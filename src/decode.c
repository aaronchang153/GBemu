#include "decode.h"

#include "opcode.h"
#include "memory.h"


void Decode_Execute(CPU *c){
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

void Decode_X_0(CPU *c){
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
                            LD_AtoMem8_addr(c, c->bc.reg);
                            break;
                        case 1: // LD (DE),A
                            LD_AtoMem8_addr(c, c->de.reg);
                            break;
                        case 2: // LD (HL+),A
                            LDI_toMem8(c);
                            break;
                        case 3: // LD (HL-),A
                            LDD_toMem8(c);
                            break;
                        default:
                            p_undef(c);
                    };
                    break;
                case 1:
                    switch(P(c->ir)){
                        case 0: // LD A,(BC)
                            LD_Mem8toA_addr(c, c->bc.reg);
                            break;
                        case 1: // LD A,(DE)
                            LD_Mem8toA_addr(c, c->de.reg);
                            break;
                        case 2: // LD A,(HL+)
                            LDI_toA(c);
                            break;
                        case 3: // LD A,(HL-)
                            LDD_toA(c);
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
                    RLCA(c);
                    break;
                case 1: // RRCA
                    RRCA(c);
                    break;
                case 2: // RLA
                    RLA(c);
                    break;
                case 3: // RRA
                    RRA(c);
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

void Decode_X_1(CPU *c){
    int z = Z(c->ir);
    int y = Y(c->ir);
    if(y == 6 && z == 6){ // HALT
        HALT(c);
    }
    else{ // LD r[y],r[z]
        if(z == 6){
            LD_Mem8toReg8_addr(c, deref_rTable(c, y), c->hl.reg);
        }
        else{
            if(y == 6){
                LD_Reg8toMem8_addr(c, c->hl.reg, deref_rTable(c, z));
            }
            else{
                LD_Reg8toReg8(c, deref_rTable(c, y), deref_rTable(c, z));
            }
        }
    }
}

void Decode_X_2(CPU *c){
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

void Decode_X_3(CPU *c){
    int y = Y(c->ir);
    int z = Z(c->ir);
    int p = P(c->ir);
    int q = Q(c->ir);

    switch(z){
        case 0:
            switch(y){
                case 0:
                case 1:
                case 2:
                case 3: // RET cc[y]
                    RET_cc(c, deref_ccTable(y));
                    break;
                case 4: // LDH (n),A
                    LDH_AtoN(c);
                    break;
                case 5: // ADD SP,d
                    ADD_SP(c);
                    break;
                case 6: // LDH A,(n)
                    LDH_NtoA(c);
                    break;
                case 7: // LD HL,SP+d
                    LD_SPtoHL(c);
                    break;
                default:
                    p_undef(c);
            };
            break;
        case 1:
            switch(q){
                case 0: // POP rp2[p]
                    POP(c, deref_rp2Table(c, p));
                    break;
                case 1:
                    switch(p){
                        case 0: // RET
                            RET(c);
                            break;
                        case 1: // RETI
                            RETI(c);
                            break;
                        case 2: // JP HL;
                            JP_HL(c);
                            break;
                        case 3: // LD SP,HL
                            LD_HLtoSP(c);
                            break;
                        default:
                            p_undef(c);
                    };
                    break;
                default:
                    p_undef(c);
            };
            break;
        case 2:
            switch(y){
                case 0:
                case 1:
                case 2:
                case 3: // JP cc[y],nn
                    JP_cc(c, deref_ccTable(y));
                    break;
                case 4: // LD (0xFF00 + C),A
                    LD_AtoC(c);
                    break;
                case 5: // LD (nn),A
                    LD_AtoMem8_imm(c);
                    break;
                case 6: // LD A,(0xFF00 + C)
                    LD_CtoA(c);
                    break;
                case 7: // LD A,(nn)
                    LD_Mem8toA_imm(c);
                    break;
                default:
                    p_undef(c);
            };
            break;
        case 3:
            switch(y){
                case 0: // JP nn
                    JP(c);
                    break;
                case 1: // CB prefix
                    CB_Prefix(c);
                    break;
                case 6: // DI
                    DI(c);
                    break;
                case 7: // EI
                    EI(c);
                    break;
                default:
                    p_undef(c);
            };
            break;
        case 4:
            if(y >= 0 && y < 4){ // CALL cc[y],nn
                CALL_cc(c, deref_ccTable(y));
            }
            else{
                p_undef(c);
            }
            break;
        case 5:
            if(q == 0){ // PUSH rp2[p]
                PUSH(c, deref_rp2Table(c, p));
            }
            else if(q == 1 && p == 0){ // CALL nn
                CALL(c);
            }
            else{
                p_undef(c);
            }
            break;
        case 6: // alu[y] n
            deref_aluMemTable(y)(c, Mem_ReadByte(c->memory, c->pc+1));
            break;
        case 7: // RST t
            RST(c);
            break;
        default:
            p_undef(c);
    };
}

void CB_Prefix(CPU *c){
    int x = X(c->ir);
    int y = Y(c->ir);
    int z = Z(c->ir);
    if(z == 6){
        switch(x){
            case 0: // rot[y] r[z]
                deref_rotMemTable(y)(c, c->hl.reg);
                break;
            case 1: // BIT y,r[z]
                BIT_Mem8(c, y, c->hl.reg);
                break;
            case 2: // RES y,r[z]
                RES_Mem8(c, y, c->hl.reg);
                break;
            case 3: // SET y,r[z]
                SET_Mem8(c, y, c->hl.reg);
                break;
            default:
                printf("Undefined CB prefixed opcode %x\n", c->ir);
        };
    }
    else{
        switch(x){
            case 0: // rot[y] r[z]
                deref_rotRegTable(y)(c, deref_rTable(c, z));
                break;
            case 1: // BIT y,r[z]
                BIT_Reg8(c, y, deref_rTable(c, z));
                break;
            case 2: // RES y,r[z]
                RES_Reg8(c, y, deref_rTable(c, z));
                break;
            case 3: // SET y,r[z]
                SET_Reg8(c, y, deref_rTable(c, z));
                break;
            default:
                printf("Undefined CB prefixed opcode %x\n", c->ir);
        };
    }
}

BYTE *deref_rTable(CPU *c, int index){
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

WORD *deref_rpTable(CPU *c, int index){
    switch(index){
        case 0: return &c->bc.reg;
        case 1: return &c->de.reg;
        case 2: return &c->hl.reg;
        case 3: return &c->sp;
        default: return NULL;
    };
}

WORD *deref_rp2Table(CPU *c, int index){
    switch(index){
        case 0: return &c->bc.reg;
        case 1: return &c->de.reg;
        case 2: return &c->hl.reg;
        case 3: return &c->af.reg;
        default: return NULL;
    };
}

BYTE deref_ccTable(int index){
    switch(index){
        case 0: return N_FLAG | Z_FLAG;
        case 1: return Z_FLAG;
        case 2: return N_FLAG | C_FLAG;
        case 3: return C_FLAG;
        default: return 0;
    };
}

ALU_OP_REG deref_aluRegTable(int index){
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

ALU_OP_MEM deref_aluMemTable(int index){
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

ROT_OP_REG deref_rotRegTable(int index){
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

ROT_OP_MEM deref_rotMemTable(int index){
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

void INC_Reg8(CPU *c, BYTE *reg){
    if((*reg & 0x0F) == 0x0F)
        CPU_SetFlag(c, H_FLAG);
    else
        CPU_ClearFlag(c, H_FLAG);
    (*reg)++;
    if(*reg == 0)
        CPU_SetFlag(c, Z_FLAG);
    else
        CPU_ClearFlag(c, Z_FLAG);
    CPU_ClearFlag(c, N_FLAG);
}

void DEC_Reg8(CPU *c, BYTE *reg){
    if((*reg & 0x0F) == 0)
        CPU_SetFlag(c, H_FLAG);
    else
        CPU_ClearFlag(c, H_FLAG);
    (*reg)--;
    if(*reg == 0)
        CPU_SetFlag(c, Z_FLAG);
    else
        CPU_ClearFlag(c, Z_FLAG);
    CPU_SetFlag(c, N_FLAG);
}