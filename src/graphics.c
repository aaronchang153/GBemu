#include "graphics.h"

#include <stdlib.h>
#include <string.h>

static BYTE MapColor(int color, BYTE palette){
    BYTE value = (palette & (0x3 << ((color - 1) * 2))) >> ((color - 1) * 2);
    if(value == 0)
        return 255;
    else if(value == 1)
        return 192;
    else if(value == 2)
        return 96;
    else
        return 0;
}


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
    BYTE control = Mem_ReadByte(g->memory, LCDC_ADDR);
    if(TEST_BIT(control, 0)){
        // Bit 0 is the BG enable
        Graphics_RenderTiles(g, control);
    }
    if(TEST_BIT(control, 1)){
        // Bit 1 is the Sprite enable
        Graphics_RenderSprites(g, control);
    }
}

void Graphics_RenderTiles(GRAPHICS *g, BYTE lcdc){
    WORD tile_data; // For location of tile informatorn
    WORD tile_map;  // For description of what tile goes where
    
    BYTE scanline = Mem_ReadByte(g->memory, LY_ADDR);
    BYTE scrollY  = Mem_ReadByte(g->memory, SCY_ADDR);
    BYTE scrollX  = Mem_ReadByte(g->memory, SCX_ADDR);
    BYTE windowY  = Mem_ReadByte(g->memory, WY_ADDR);
    BYTE windowX  = Mem_ReadByte(g->memory, WY_ADDR) - 7;

    bool tileID_signed = false;

    bool window = false;
    if(TEST_BIT(lcdc, 5) && windowY <= scanline){
        window = true;
    }

    if(TEST_BIT(lcdc, 4)){
        tile_data = 0x8000;
    }
    else{
        tile_data = 0x8800;
        tileID_signed = true;
    }

    if(!window){
        if(TEST_BIT(lcdc, 3)){
            tile_map = 0x9C00;
        }
        else{
            tile_map = 0x9800;
        }
    }

    BYTE yPos;
    BYTE xPos;
    if(!window){
        yPos = scrollY + scanline;
    }
    else{
        yPos = scanline - windowY;
    }

    WORD tile_row = (yPos / 8) * 32;
    WORD tile_col;
    WORD tile_addr;
    WORD tile_data_addr;
    SIGNED_WORD tileID;

    for(int pixel = 0; pixel < SCREEN_WIDTH; pixel++){
        xPos = pixel + scrollX;
        if(window && pixel >= windowX){
            xPos = pixel - windowX;
        }

        tile_col = xPos / 8;
        tile_addr = tile_map + tile_row + tile_col;
        if(tileID_signed){
            tileID = (SIGNED_BYTE) Mem_ReadByte(g->memory, tile_addr);
        }
        else{
            tileID = Mem_ReadByte(g->memory, tile_addr);
        }

        tile_data_addr = tile_data;
        if(tileID_signed){
            tile_data_addr += (tileID + 128) * 16;
        }
        else{
            tile_data_addr += tileID * 16;
        }

        BYTE line = (yPos % 8) * 2;
        BYTE color_data1 = Mem_ReadByte(g->memory, line);
        BYTE color_data2 = Mem_ReadByte(g->memory, line + 1);

        // Pixel 0 maps to bit 7, pixel 1 to bit 6, etc.
        SIGNED_BYTE color_bit = 0 - ((xPos % 8) - 7);

        int color_num = ((color_data2 & (0x01 << color_bit)) == 0) ? 0x00 : 0x02;
        color_num |= ((color_data1 & (0x01 << color_bit)) == 0) ? 0x00 : 0x01;

        g->frame_buffer[pixel][scanline].r = 
            g->frame_buffer[pixel][scanline].g = 
            g->frame_buffer[pixel][scanline].b = MapColor(color_num, Mem_ReadByte(g->memory, BGP_ADDR));
    }
}

void Graphics_RenderSprites(GRAPHICS *g, BYTE lcdc){

}