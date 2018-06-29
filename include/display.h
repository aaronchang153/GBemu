#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"
#include "SDL.h"

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 144

#define WINDOW_NAME "GBemu"

#define PIXEL_SIZE 2

static const unsigned int TRANSPARENT   = 0x00000000;
static const unsigned int WHITE         = 0xFFFFFFFF;
static const unsigned int LIGHT_GRAY    = 0xC0C0C0FF;
static const unsigned int DARK_GRAY     = 0x606060FF;
static const unsigned int BLACK         = 0x000000FF;

typedef struct{
    union{
        struct{
            BYTE a;
            BYTE b;
            BYTE g;
            BYTE r;
        };
        unsigned int color;
    };
} PIXEL;

typedef struct{
    SDL_Window *window;
    SDL_Renderer *renderer;
} DISPLAY;


DISPLAY *Display_Create();

void Display_Destroy(DISPLAY *d);

void Display_RenderScreen(DISPLAY *d, PIXEL frame[SCREEN_WIDTH][SCREEN_HEIGHT]);

#endif // DISPLAY_H