#include "gameboy.h"

#include <stdio.h>

#ifdef DEBUG
#include "cpudebug.h"
#endif // DEBUG


int main(){
    char game_file[128];
    printf("Enter path to game: ");
    scanf("%s", game_file);
    GAMEBOY *gb = GB_Create();
    GB_LoadGame(gb, game_file);
    GB_Startup(gb);
#ifdef DEBUG
    Start_Debugger(gb->cpu);
#else
    while(true){
        GB_Update(gb);
    }
#endif // DEBUG
    GB_Destroy(gb);
    return 0;
}