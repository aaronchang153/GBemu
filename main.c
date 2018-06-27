#include "gameboy.h"
#include "SDL.h"

#include <stdio.h>

#ifdef DEBUG
#include "gbdebug.h"
#endif // DEBUG


int SDL_main(int argc, char *argv[]){
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    char game_file[128];
    printf("Enter path to game: ");
    scanf("%s", game_file);
    GAMEBOY *gb = GB_Create();
    GB_LoadGame(gb, game_file);
    GB_Startup(gb);
#ifdef DEBUG
    Start_Debugger(gb);
#else
    SDL_Event event;
    bool running = true;
    while(running){
        if(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                    running = false;
                    break;
            };
        }
        GB_Update(gb);
        SDL_Delay(30);
    }
#endif // DEBUG
    GB_Destroy(gb);
    SDL_Quit();
    return 0;
}