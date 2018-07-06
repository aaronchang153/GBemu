#include "gbdebug.h"

#ifdef DEBUG

#include "disassemble.h"

#include <stdio.h>
#include <string.h>

static const int CYCLES_PER_UPDATE = CLK_F / UPDATES_PER_SEC;
static void Get_Command(GAMEBOY *gb, int *counter, int *bp, bool *cont, bool *running, bool *restart);

static int power(int x, int n){
    if(n == 0)
        return 1;
    return x * power(x, n-1);
}

static int Parse_Hex(char *in){
    if(in == NULL){
        return 0;
    }
    else{
        int result = 0;
        int place = 0;
        for(int i = strlen(in) - 1; i >= 0; i--){
            if(in[i] == '\n')
                continue;
            else if(in[i] >= 65 && in[i] <= 70) // A-F
                result += (in[i] - 55) * power(16, place);
            else if(in[i] >= 97 && in[i] <= 102) // a-f
                result += (in[i] - 87) * power(16, place);
            else if(in[i] >= 48 && in[i] <= 57) // 0-9
                result += (in[i] - 48) * power(16, place);
            place++;
        }
        return result;
    }
}

static void Print_State(CPU *c){
    printf("==================================================\n");
    printf("OPCODE: 0x%02x\t\tDISASSEMBLY: %s\n", c->ir, Disassemble_Instruction(c));
    printf("REGISTERS:\n");
    printf("\tPC: 0x%04x\n", c->pc);
    printf("\tSP: 0x%04x\n", c->sp);
    printf("\tA: 0x%02x\t\tF: 0x%02x\n", c->af.hi, c->af.lo);
    printf("\tB: 0x%02x\t\tC: 0x%02x\n", c->bc.hi, c->bc.lo);
    printf("\tD: 0x%02x\t\tE: 0x%02x\n", c->de.hi, c->de.lo);
    printf("\tH: 0x%02x\t\tL: 0x%02x\n", c->hl.hi, c->hl.lo);
    printf("FLAGS:\n");
    printf("\tZ:   %d\n", CPU_CheckFlag(c, Z_FLAG));
    printf("\tN:   %d\n", CPU_CheckFlag(c, N_FLAG));
    printf("\tH:   %d\n", CPU_CheckFlag(c, H_FLAG));
    printf("\tC:   %d\n", CPU_CheckFlag(c, C_FLAG));
    printf("\tIME: %d\n", c->IME);
    printf("MISC:\n");
    printf("\tIMM8:  0x%02x\n", Mem_ReadByte(c->memory, c->pc+1));
    printf("\tIMM16: 0x%04x\n", Mem_ReadWord(c->memory, c->pc+1));
    //printf("\tMEM[HL-1]: 0x%02x\n", Mem_ReadByte(c->memory, c->hl.reg-1));
    printf("\tMEM[HL]  : 0x%02x\n", Mem_ReadByte(c->memory, c->hl.reg));
    //printf("\tMEM[HL+1]: 0x%02x\n", Mem_ReadByte(c->memory, c->hl.reg+1));
    printf("==================================================\n");
}

static void Dump_Memory(CPU *c){
    FILE *fp = fopen("Mem_Dump.txt", "w");
    MEM_REGION region;
    BYTE content;
    if(fp != NULL){
        for(int i = 0; i < 0x10000; i++){
            region = Mem_GetRegion(c->memory, (WORD) i);
            if(region == ROM0 || region == ROMX)
                content = (c->memory->cartridge == NULL) ? 0 : Cartridge_ReadROM(c->memory->cartridge, i);
            else if(region == SRAM)
                content = (c->memory->cartridge == NULL) ? 0 : Cartridge_ReadRAM(c->memory->cartridge, i);
            else
                content = Mem_ReadByte(c->memory, i);
            fprintf(fp, "0x%04X:\t0x%02X\n", i, content);
        }
        fclose(fp);
        printf("Memory successfully dumped.\n");
    }
}

void Enter_Debug_Mode(GAMEBOY *gb){
    char input;
    while(true){
        unsigned int total_cycles = 0;
        unsigned int cycles;

        while(total_cycles < CYCLES_PER_UPDATE){
            CPU_Fetch(gb->cpu);
            Print_State(gb->cpu);
            input = getchar();
            if(input == 'q'){
                break;
            }
            else if(input == 'D'){
                Dump_Memory(gb->cpu);
            }
            CPU_DecodeExecute(gb->cpu);
            cycles = CPU_GetCycles(gb->cpu);
            total_cycles += cycles;
            Timer_Update(gb->timer, cycles);
            Graphics_Update(gb->graphics, cycles);
            Interrupt_Handle(gb->cpu);
        }
        Graphics_RenderScreen(gb->graphics);
        if(total_cycles < CYCLES_PER_UPDATE){
            break;
        }
    }
}

void Start_Debugger(GAMEBOY *gb){
    int bp;
    int counter = 0;
    bool cont = false;
    bool restart = false;
    unsigned int total_cycles = 0;
    unsigned int cycles;
    unsigned int instructions_executed = 0;
    SDL_Event event;
    bool running = true;
    while(running){
        while(SDL_PollEvent(&event)){
            // For the sake of time, handle all pending events at once when debugging
            switch(event.type){
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if(Joypad_SetState(gb->joypad, event, JOYPAD_PRESSED, Mem_ReadByte(gb->memory, P1_ADDR))){
                        // If there's a joypad interrupt
                        Mem_RequestInterrupt(gb->memory, IF_JOYPAD);
                    }
                    break;
                case SDL_KEYUP:
                    Joypad_SetState(gb->joypad, event, JOYPAD_NOT_PRESSED, Mem_ReadByte(gb->memory, P1_ADDR));
                    break;
            };
        }
        total_cycles = 0;
        while(total_cycles < CYCLES_PER_UPDATE){
            if(!gb->cpu->stop){
                CPU_Fetch(gb->cpu);

                if(gb->cpu->pc == bp)
                    cont = false;
                if(!cont || counter == 1){
                    cont = false;
                    Print_State(gb->cpu);
                    Get_Command(gb, &counter, &bp, &cont, &running, &restart);
                    if(restart){
                        restart = false;
                        continue;
                    }
                    if(!running)
                        break;
                }
                counter = (counter <= 0) ? counter : counter - 1;

                if(!gb->cpu->halt){
                    CPU_DecodeExecute(gb->cpu);
                    cycles = CPU_GetCycles(gb->cpu);
                    total_cycles += cycles;
                    instructions_executed++;
                }
                else{
                    cycles = 4;
                    total_cycles += 4;
                } // endif halt
                Timer_Update(gb->timer, cycles);
                Graphics_Update(gb->graphics, cycles);
            }
            else{
                cycles = 4;
                total_cycles += 4;
            } // endif stop
            Interrupt_Handle(gb->cpu);
        }
        Graphics_RenderScreen(gb->graphics);
        if(total_cycles < CYCLES_PER_UPDATE){
            break;
        }
        //SDL_Delay(30);
    }
    Dump_Memory(gb->cpu);
}

static void Get_Command(GAMEBOY *gb, int *counter, int *bp, bool *cont, bool *running, bool *restart){
    char input[16];
    char *tok;
    fgets(input, 16, stdin);
    if(input[0] == 'q'){
        *running = false;
    }
    else if(input[0] == 'D'){
        Dump_Memory(gb->cpu);
    }
    else if(input[0] == 'b'){
        tok = strtok(input, " ");
        tok = strtok(NULL, " ");
        *bp = Parse_Hex(tok);
        *cont = true;
    }
    else if(input[0] == 'e'){
        tok = strtok(input, " ");
        tok = strtok(NULL, " ");
        *counter = Parse_Hex(tok);
        *cont = true;
    }
    else if(input[0] == 'C'){
        *bp = -1;
        *cont = true;
    }
    else if(input[0] == 'R'){
        *restart = true;
        GB_Startup(gb);
    }
}

#endif // DEBUG