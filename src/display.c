#include "display.h"

#include <stdio.h>
#include <stdlib.h>


DISPLAY *Display_Create(){
    DISPLAY *display = malloc(sizeof(DISPLAY));
    if(display != NULL){
        display->window = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                SCREEN_WIDTH * PIXEL_SIZE, SCREEN_HEIGHT * PIXEL_SIZE, SDL_WINDOW_SHOWN);
        if(display->window == NULL){
            printf("Unable to create window.\n");
            Display_Destroy(display);
            display = NULL;
        }
        else{
            display->renderer = SDL_CreateRenderer(display->window, -1, SDL_RENDERER_ACCELERATED);
            if(display->renderer == NULL){
                printf("Unable to create renderer.\n");
                Display_Destroy(display);
                display = NULL;
            }
        }
    }
    return display;
}

void Display_Destroy(DISPLAY *d){
    if(d != NULL){
        SDL_DestroyRenderer(d->renderer);
        SDL_DestroyWindow(d->window);
        free(d);
    }
}

void Display_RenderScreen(DISPLAY *d, PIXEL frame[SCREEN_WIDTH][SCREEN_HEIGHT]){
    int i, j;
    SDL_Rect pixel;

    for(j = 0; j < SCREEN_HEIGHT; j++){
        for(i = 0; i < SCREEN_WIDTH; i++){
            SDL_SetRenderDrawColor(d->renderer, frame[i][j].r, frame[i][j].g, frame[i][j].b, frame[i][j].a);
            pixel.x = i * PIXEL_SIZE;
            pixel.y = j * PIXEL_SIZE;
            pixel.w = PIXEL_SIZE;
            pixel.h = PIXEL_SIZE;

            SDL_RenderFillRect(d->renderer, &pixel);
        }
    }
    SDL_RenderPresent(d->renderer);
}