#include "graphics.h"

#include <stdlib.h>
#include <string.h>

static unsigned int MapColor(int color, BYTE palette){
    BYTE value = (palette >> (color * 2)) & 0x03;
    if(value == 0)
        return WHITE;
    else if(value == 1)
        return LIGHT_GRAY;
    else if(value == 2)
        return DARK_GRAY;
    else
        return BLACK;
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

void Graphics_SetDisplay(GRAPHICS *g, DISPLAY *d){
    g->display = d;
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

void Graphics_RenderScreen(GRAPHICS *g){
    Display_RenderScreen(g->display, g->frame_buffer);
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
        bool interrupt = false;

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
    /**
     * One tile is 8px X 8px
     * Entire screen is 256 px(32 tiles) X 256 px(32 tiles)
     * Visible screen is 160 px(20 tiles) X 144 px (18 tiles)
     * Each tile in memory occupies 16 bytes (2 bytes/tile)
     */
    WORD tile_data; // For location of tile informatorn
    WORD tile_map;  // For description of what tile goes where
    
    BYTE scanline = Mem_ReadByte(g->memory, LY_ADDR) - 1;
    BYTE scrollY  = Mem_ReadByte(g->memory, SCY_ADDR);
    BYTE scrollX  = Mem_ReadByte(g->memory, SCX_ADDR);
    BYTE windowY  = Mem_ReadByte(g->memory, WY_ADDR);
    BYTE windowX  = Mem_ReadByte(g->memory, WX_ADDR) - 7;

    bool tileID_signed = false;
    bool window = false;

    if(TEST_BIT(lcdc, 5) && windowY <= scanline){
        window = true;
    }

    if(TEST_BIT(lcdc, 4)){ // Tile data 0x8000-0x8FFF
        tile_data = 0x8000;
    }
    else{ // Tile data 0x8800-0x97FF
        tile_data = 0x8800;
        tileID_signed = true;
    }

    if(window){
        tile_map = (TEST_BIT(lcdc, 6)) ? 0x9C00 : 0x9800;
    }
    else{
        tile_map = (TEST_BIT(lcdc, 3)) ? 0x9C00 : 0x9800;
    }

    BYTE yPos;
    BYTE xPos;
    if(!window){
        yPos = scrollY + scanline;
    }
    else{
        yPos = scanline - windowY;
    }

    // x32 because there's 32 tiles per row
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
        BYTE color_data1 = Mem_ReadByte(g->memory, tile_data_addr + line);
        BYTE color_data2 = Mem_ReadByte(g->memory, tile_data_addr + line + 1);

        // Pixel 0 maps to bit 7, pixel 1 to bit 6, etc.
        int color_bit = ~((xPos % 8) - 7) + 1;

        int color_num = (TEST_BIT(color_data2, color_bit)) ? 0x02 : 0x00;
        color_num += (TEST_BIT(color_data1, color_bit)) ? 0x01 : 0x00;

        g->frame_buffer[pixel][scanline].color = MapColor(color_num, Mem_ReadByte(g->memory, BGP_ADDR));
    }
}

void Graphics_RenderSprites(GRAPHICS *g, BYTE lcdc)
{
    // Double height sprites are 8x16 (as opposed to 8x8)
    bool double_height = false;

    BYTE scanline = Mem_ReadByte(g->memory, LY_ADDR) - 1;

    if (TEST_BIT(lcdc, 2))
        double_height = true;

    for (int sprite = 0; sprite < 40; sprite++)
    {
        // sprite occupies 4 bytes in the sprite attributes table
        BYTE index = sprite * 4;
        BYTE yPos = Mem_ReadByte(g->memory, 0xFE00 + index) - 16;
        BYTE xPos = Mem_ReadByte(g->memory, 0xFE00 + index + 1) - 8;
        BYTE tileLocation = Mem_ReadByte(g->memory, 0xFE00 + index + 2);
        BYTE attributes = Mem_ReadByte(g->memory, 0xFE00 + index + 3);

        bool yFlip = TEST_BIT(attributes, 6);
        bool xFlip = TEST_BIT(attributes, 5);

        int ysize = double_height ? 16 : 8;

        // does this sprite intercept with the scanline?
        if ((scanline >= yPos) && (scanline < (yPos + ysize)))
        {
            // sprite's line number
            int line = scanline - yPos;

            // read the sprite in backwards in the y axis
            if (yFlip)
                line = ~(line - ysize) + 1;

            line *= 2; // 2 bytes/row
            WORD dataAddress = 0x8000 + (tileLocation * 16) + line;
            BYTE color_data1 = Mem_ReadByte(g->memory, dataAddress);
            BYTE color_data2 = Mem_ReadByte(g->memory, dataAddress + 1);

            // read colors from bit 7 to 0
            for (int tilePixel = 7; tilePixel >= 0; tilePixel--)
            {
                int colorbit = tilePixel;
                if (xFlip)
                    colorbit = ~(colorbit - 7) + 1;

                // the rest is the same as for tiles
                int colorNum = TEST_BIT(color_data2, colorbit) ? 0x02 : 0x00;
                colorNum += TEST_BIT(color_data1, colorbit) ? 0x01 : 0x00;

                WORD colorAddress = TEST_BIT(attributes, 4) ? 0xFF49 : 0xFF48;
                unsigned int color = MapColor(colorNum, colorAddress);

                // white is transparent for sprites.
                if (color != WHITE){
                    g->frame_buffer[xPos - tilePixel + 7][scanline].color = color;
                }
/*
                int xPix = 0 - tilePixel;
                xPix += 7;

                int pixel = xPos + xPix;

                g->frame_buffer[pixel][scanline].r = 
                    g->frame_buffer[pixel][scanline].g = 
                    g->frame_buffer[pixel][scanline].b = col;
*/
            }
        }
    }
}