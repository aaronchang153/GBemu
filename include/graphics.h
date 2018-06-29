#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "common.h"
#include "display.h"
#include "memory.h"

#define VBLANK_END    153

#define CLK_PER_SCANLINE 456

#define MODE_HBLANK     0
#define MODE_VBLANK     1
#define MODE_SEARCH_OAM 2
#define MODE_TRANSFER   3

// Note: These end points are only valid when counting 
//       down from the clocks per scanline.
// OAM search mode lasts the first 80 clock cycles, lcd 
// transfer mode takes the next 172 clock cycles, and 
// HBLANK (not listed) takes the remining 204 clock cycles.
#define MODE_SEARCH_END   (CLK_PER_SCANLINE - 80)
#define MODE_TRANSFER_END (MODE_SEARCH_END - 172)

// LY_ADDR 0xFF44 defined in memory.h
#define LCDC_ADDR   0xFF40
#define STAT_ADDR   0xFF41
#define SCY_ADDR    0xFF42
#define SCX_ADDR    0xFF43
#define LYC_ADDR    0xFF45
#define BGP_ADDR    0xFF47
#define OBP0_ADDR   0xFF48
#define OBP1_ADDR   0xFF49
#define WY_ADDR     0xFF4A
#define WX_ADDR     0xFF4B


typedef struct{
    int scanline_counter;
    PIXEL frame_buffer[SCREEN_WIDTH][SCREEN_HEIGHT];
    MEMORY *memory;
    DISPLAY *display;
} GRAPHICS;


GRAPHICS *Graphics_Create();

void Graphics_Destroy(GRAPHICS *g);

void Graphics_SetMemory(GRAPHICS *g, MEMORY *mem);

void Graphics_SetDisplay(GRAPHICS *g, DISPLAY *d);

void Graphics_Update(GRAPHICS *g, int cycles);

void Graphics_RenderScreen(GRAPHICS *g);

bool Graphics_LCDEnabled(GRAPHICS *g);

void Graphics_UpdateLCDSTAT(GRAPHICS *g);

void Graphics_DrawScanline(GRAPHICS *g);

void Graphics_RenderTiles(GRAPHICS *g, BYTE lcdc);

void Graphics_RenderSprites(GRAPHICS *g, BYTE lcdc);

#endif // GRAPHICS_H