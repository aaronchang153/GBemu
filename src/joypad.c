#include "joypad.h"

#include <stdlib.h>


static int MapKeyPress(SDL_Event e);

JOYPAD *Joypad_Create(){
    JOYPAD *joypad = malloc(sizeof(JOYPAD));
    if(joypad != NULL){
        joypad->state = 0xFF;
    }
    return joypad;
}

void Joypad_Destroy(JOYPAD *j){
    if(j != NULL){
        free(j);
    }
}

BYTE Joypad_GetState(JOYPAD *j, BYTE p1){
    if(!TEST_BIT(p1, 4))
        return (p1 & 0xF0) | ((j->state & 0xF0) >> 4);
    else if(!TEST_BIT(p1, 5))
        return (p1 & 0xF0) | (j->state & 0x0F);
    else
        return p1;
}

bool Joypad_SetState(JOYPAD *j, SDL_Event e, int state, BYTE p1){
    bool interrupt = false;
    int key = MapKeyPress(e);
    if(key >= 0){
        if(state == JOYPAD_PRESSED && TEST_BIT(j->state, key)){
            // If the key is going from unpressed to pressed
            if(((key > 3) && TEST_BIT(p1, 4)) || ((key >= 3) && TEST_BIT(p1, 5)))
                interrupt = true;
        }
        // Clear corresponding bit if pressed, otherwise set it to 1
        j->state = (state == JOYPAD_PRESSED) ? (j->state & ~(0x01 << key)) : (j->state | (0x01 << key));
    }
    return interrupt;
}

static int MapKeyPress(SDL_Event e){
    switch(e.key.keysym.scancode){
        case SDL_SCANCODE_K: // A
            return 0;
        case SDL_SCANCODE_J: // B
            return 1;
        case SDL_SCANCODE_V: // SELECT
            return 2;
        case SDL_SCANCODE_B: // START
            return 3;
        case SDL_SCANCODE_D: // RIGHT
            return 4;
        case SDL_SCANCODE_A: // LEFT
            return 5;
        case SDL_SCANCODE_W: // UP
            return 6;
        case SDL_SCANCODE_S: // DOWN
            return 7;
        default:
            return -1;
    };
}