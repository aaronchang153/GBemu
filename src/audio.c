#include "audio.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define SAMPLE_FREQUENCY     44100 // Hz
#define FRAME_SEQUENCER_FREQ   512 // Hz

#define MAX_VOLUME 2000

const int CYCLES_PER_SAMPLE = CLK_F / SAMPLE_FREQUENCY;
const int CYCLES_PER_FRAME  = CLK_F / FRAME_SEQUENCER_FREQ;

static const float square_wave_table[4][8] = 
{   // Same type as the audio data buffer and audio spec format
    {-1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0}, // 12.5% duty
    {-1.0, -1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0}, // 25% duty
    {-1.0, -1.0, -1.0, -1.0,  1.0,  1.0,  1.0,  1.0}, // 50% duty
    {-1.0, -1.0, -1.0, -1.0, -1.0, -1.0,  1.0,  1.0}  // 75% duty
};

static inline int us_to_cycles(float us) { return (int) round(us * CLK_F); }

static void Update_Ch1(APU *a, int cycles); // Square wave with sweep and envelope
static void Update_Ch2(APU *a, int cycles); // Square wave with envelope
static void Update_Ch3(APU *a, int cycles); // Arbitrary waveform
static void Update_Ch4(APU *a, int cycles); // White noise

APU *APU_Create(){
    APU *apu = malloc(sizeof(APU));
    if(apu != NULL){
        memset(apu, 0, sizeof(APU));
        SDL_AudioSpec want;
        want.freq = SAMPLE_FREQUENCY;
        want.format = AUDIO_F32LSB;
        want.channels = 2; // Stereo Sound
        want.callback = NULL;
        SDL_OpenAudio(&want, &apu->audio_spec);
        if(apu->audio_spec.freq != want.freq || apu->audio_spec.format != want.format){
            APU_Destroy(apu);
            apu = NULL;
        }
        else{
            apu->sample_timer = CYCLES_PER_SAMPLE;
            apu->sequence[0] = apu->sequence[1] = 0;
            srand(time(NULL)); // Random number needed for noise on channel 4
        }
    }
    return apu;
}

void APU_Destroy(APU *a){
    SDL_CloseAudio();
    if(a != NULL){
        free(a);
    }
}

void APU_SetMemory(APU *a, MEMORY *mem){
    if(a != NULL){
        a->memory = mem;
    }
}

void APU_Update(APU *a, int cycles){
    if(a != NULL){
        a->sample_timer -= cycles;
        if(a->sample_timer <= 0){
            a->sample_timer += CYCLES_PER_SAMPLE;
            a->audio_buffer[a->sample_number] = 0.0;
            a->audio_buffer[a->sample_number + 1] = 0.0;

            Update_Ch1(a, cycles);
            Update_Ch2(a, cycles);
            Update_Ch3(a, cycles);
            Update_Ch4(a, cycles);
            
            a->sample_number += 2;
            if(a->sample_number == AUDIO_BUFFER_LENGTH){
                SDL_QueueAudio(1, a->audio_buffer, AUDIO_BUFFER_LENGTH);
                a->sample_number = 0;
            }
        }
    }
}

static void Update_Ch1(APU *a, int cycles){
    /***** Incomplete *****/
    int frequency = Mem_ReadByte(a->memory, NR13_ADDR);
    frequency |= (Mem_ReadByte(a->memory, NR14_ADDR) & 0x7) << 8;
    a->frame_countdown[0] = (2048 - frequency) * 4;
    a->frame_timer[0]--;
    if(a->frame_timer == 0){
        a->frame_timer[0] = a->frame_countdown[0];
        a->sequence[0] = (a->sequence[0] + 1) % 8;
    }
}

static void Update_Ch2(APU *a, int cycles){
    int frequency = Mem_ReadByte(a->memory, NR23_ADDR);
    frequency |= (Mem_ReadByte(a->memory, NR24_ADDR) & 0x7) << 8;
    a->frame_countdown[1] = (2048 - frequency) * 4;
    int wave_duty = Mem_ReadByte(a->memory, NR21_ADDR) >> 6;
    a->frame_timer[1]--;
    if(a->frame_timer == 0){
        a->frame_timer[1] = a->frame_countdown[1];
        a->sequence[1] = (a->sequence[1] + 1) % 8;
    }
}

static void Update_Ch3(APU *a, int cycles){

}

static void Update_Ch4(APU *a, int cycles){

}
