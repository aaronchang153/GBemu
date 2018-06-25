#include "graphics.h"

#include <stdlib.h>
#include <string.h>


GRAPHICS *Graphics_Create(){
    GRAPHICS *graphics = malloc(sizeof(GRAPHICS));
    if(graphics != NULL){
        memset(graphics, 0, sizeof(GRAPHICS));
        graphics->scanline_counter = CLK_PER_SCANLINE;
    }
    return graphics;
}

void Graphics_Destroy(GRAPHICS *g){
    if(g != NULL){
        free(g);
    }
}

void Graphics_SetMemory(GRAPHICS *g, MEMORY *mem){
    g->memory = mem;
}

void Graphics_Update(GRAPHICS *g, int cycles){
    Graphics_UpdateLCDSTAT(g);

    if(Graphics_LCDEnabled(g)){
        g->scanline_counter -= cycles;

        if(g->scanline_counter <= 0){
            // Move to next scanline
            BYTE current_line = Mem_ReadByte(g->memory, LY_ADDR);
            Mem_ForceWrite(g->memory, LY_ADDR, ++current_line);
            // Reset scanline_counter
            g->scanline_counter = CLK_PER_SCANLINE;

            if(current_line == SCREEN_HEIGHT){
                // End of visible screen. Request VBLANK interrupt
                Mem_RequestInterrupt(g->memory, IF_VBLANK);
            }
            else if(current_line > VBLANK_END){
                Mem_ForceWrite(g->memory, LY_ADDR, 0x00);
            }
            else if(current_line < SCREEN_HEIGHT){
                Graphics_DrawScanline(g);
            }
        }
    }
}

bool Graphics_LCDEnabled(GRAPHICS *g){
    return TEST_BIT(Mem_ReadByte(g->memory, LCDC_ADDR), 7);
}

void Graphics_UpdateLCDSTAT(GRAPHICS *g){
    BYTE status = Mem_ReadByte(g->memory, STAT_ADDR);
    if(!Graphics_LCDEnabled(g)){
        // Reset scanline
        g->scanline_counter = CLK_PER_SCANLINE;
        Mem_ForceWrite(g->memory, LY_ADDR, 0x00);
        // Set LCD to mode 1
        status &= 0xFC; // (1111 1100)_base2
        status |= 0x01;
        Mem_ForceWrite(g->memory, STAT_ADDR, status);
    }
    else{
        BYTE current_line = Mem_ReadByte(g->memory, LY_ADDR);
        BYTE current_mode = status & 0x03;

        BYTE next_mode;
        bool interrupt;

        if(current_line >= SCREEN_HEIGHT){
            next_mode = MODE_VBLANK;
            status &= 0xFC;
            status |= 0x01;
            interrupt = TEST_BIT(status, 5);
        }
        else{
            if(g->scanline_counter >= MODE_SEARCH_END){
                next_mode = MODE_SEARCH_OAM;
                status &= 0xFC;
                status |= 0x02;
                interrupt = TEST_BIT(status, 5);
            }
            else if(g->scanline_counter >= MODE_TRANSFER_END){
                next_mode = MODE_TRANSFER;
                status &= 0xFC;
                status |= 0x03;
            }
            else{
                next_mode = MODE_HBLANK;
                status &= 0xFC;
                interrupt = TEST_BIT(status, 3);
            }
        }
        
        if(interrupt && (next_mode != current_mode)){
            Mem_RequestInterrupt(g->memory, IF_LCD_STAT);
        }
        if(Mem_ReadByte(g->memory, LY_ADDR) == Mem_ReadByte(g->memory, LYC_ADDR)){
            status |= 0x04; // Set bit 2
            if(TEST_BIT(status, 6)){
                Mem_RequestInterrupt(g->memory, IF_LCD_STAT);
            }
        }
        else{
            status &= ~(0x04); // Reset bit 2
        }
        Mem_ForceWrite(g->memory, STAT_ADDR, status);
    }
}

void Graphics_DrawScanline(GRAPHICS *g){

}