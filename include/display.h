#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"
#include "SDL.h"

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 144

#define WINDOW_NAME "GBemu"

#define PIXEL_SIZE 2

typedef struct{
    BYTE r;
    BYTE g;
    BYTE b;
} PIXEL;

typedef struct{
    SDL_Window *window;
    SDL_Renderer *renderer;
} DISPLAY;


DISPLAY *Display_Create();

void Display_Destroy(DISPLAY *d);

void Display_RenderScreen(DISPLAY *d, PIXEL frame[SCREEN_WIDTH][SCREEN_HEIGHT]);

#endif // DISPLAY_H