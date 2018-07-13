#ifndef JOYPAD_H
#define JOYPAD_H

#include "common.h"
#include <SDL2/SDL.h>

#define JOYPAD_PRESSED      0
#define JOYPAD_NOT_PRESSED  1

typedef struct{
    union{
        struct{
            BYTE a      : 1;
            BYTE b      : 1;
            BYTE select : 1;
            BYTE start  : 1;
            BYTE right  : 1;
            BYTE left   : 1;
            BYTE up     : 1;
            BYTE down   : 1;
        };
        BYTE state;
    };
} JOYPAD;

JOYPAD *Joypad_Create();

void Joypad_Destroy(JOYPAD *j);

BYTE Joypad_GetState(JOYPAD *j, BYTE p1);

// Returns true if there's a Joypad Interrupt and false otherwise
bool Joypad_SetState(JOYPAD *j, SDL_Event e, int state, BYTE p1);

#endif // JOYPAD_H