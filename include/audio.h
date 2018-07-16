#ifndef AUDIO_H
#define AUDIO_H

#include "common.h"
#include "memory.h"

#include <stdint.h>
#include <SDL2/SDL.h>

/**
 * The GameBoy has 4 sound channels:
 *  1. Square wave with sweep and envelope functions
 *  2. Square wave with envelope function
 *  3. Arbitrary waveform from wave pattern RAM
 *  4. White noise with evelope function
 */

#define NR10_ADDR 0xFF10
#define NR11_ADDR 0xFF11
#define NR12_ADDR 0xFF12
#define NR13_ADDR 0xFF13
#define NR14_ADDR 0xFF14

#define NR21_ADDR 0xFF16
#define NR22_ADDR 0xFF17
#define NR23_ADDR 0xFF18
#define NR24_ADDR 0xFF19

#define NR30_ADDR 0xFF1A
#define NR31_ADDR 0xFF1B
#define NR32_ADDR 0xFF1C
#define NR33_ADDR 0xFF1D
#define NR34_ADDR 0xFF1E

#define NR41_ADDR 0xFF20
#define NR42_ADDR 0xFF21
#define NR43_ADDR 0xFF22
#define NR44_ADDR 0xFF23

#define NR50_ADDR 0xFF24 // Channel Control + On-Off + Volume
#define NR51_ADDR 0xFF25 // Sound ouput terminal select
#define NR52_ADDR 0xFF26 // Sound on/off

#define WAVE_PATTERN_RAM 0xFF30 // $FF30 - $FF3F

#define AUDIO_BUFFER_LENGTH 0x800

typedef struct{
    SDL_AudioSpec audio_spec;
    float audio_buffer[AUDIO_BUFFER_LENGTH]; // samples stored as {left-channel, right-channel}
    int sample_number; // current spot in audio_buffer
    int sample_timer; // only sample audio when this timer reaches 0 (cycles)
    int sound_timer[4]; // in cycles
    uint8_t sequence[2]; // Both square wave channels have 8 states depending on the duty
    int frame_countdown[2]; // frame_timer is set to this value when it hits 0
    int frame_timer[2]; // the corresponding square wave sequence is incremented when this hits 0
    MEMORY *memory;
} APU;


APU *APU_Create();

void APU_Destroy(APU *a);

void APU_SetMemory(APU *a, MEMORY *mem);

void APU_Update(APU *a, int cycles);

#endif // AUDIO_H
