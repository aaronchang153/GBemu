#include "audio.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define SAMPLE_FREQUENCY     44100 // Hz
#define FRAME_SEQUENCER_FREQ   512 // Hz

// Volume needs to be normalized to [-1.0, 1.0] when using F32 audio format
#define MAX_VOLUME 3000
#define MAX_OUTPUT_VOLUME (MAX_VOLUME / 6)
#define MAX_CHANNEL_VOLUME (MAX_OUTPUT_VOLUME / 4)

#define ADJUST_VOLUME(sample, factor) ((sample) *= ((MAX_OUTPUT_VOLUME * (factor)) / 7))

#define NORMALIZE_SAMPLE(sample) ((sample) /= MAX_VOLUME)

const int CYCLES_PER_SAMPLE = CLK_F / SAMPLE_FREQUENCY;
const int CYCLES_PER_FRAME  = CLK_F / FRAME_SEQUENCER_FREQ;

static const AudioSample square_wave_table[4][8] =
{   // Same type as the audio data buffer and audio spec format
    {1, 0, 0, 0, 0, 0, 0, 0}, // 12.5% duty
    {1, 1, 0, 0, 0, 0, 0, 0}, // 25% duty
    {1, 1, 1, 1, 0, 0, 0, 0}, // 50% duty
    {1, 1, 1, 1, 1, 1, 0, 0}  // 75% duty
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
#ifdef FLOAT32_AUDIO
        want.format = AUDIO_F32SYS;
#else
        want.format = AUDIO_S16SYS;
#endif // FLOAT32_AUDIO
        want.freq = SAMPLE_FREQUENCY;
        want.channels = 2; // Stereo Sound
        want.samples = AUDIO_BUFFER_LENGTH;
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
            SDL_PauseAudio(0);
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
            a->audio_buffer[a->sample_number] = 0;
            a->audio_buffer[a->sample_number + 1] = 0;
            a->nr51 = Mem_ReadByte(a->memory, NR51_ADDR);

            BYTE nr50 = Mem_ReadByte(a->memory, NR50_ADDR);

            Update_Ch1(a, cycles);
            Update_Ch2(a, cycles);
            Update_Ch3(a, cycles);
            Update_Ch4(a, cycles);

            ADJUST_VOLUME(a->audio_buffer[a->sample_number], (nr50 & 0x70) >> 4);
            ADJUST_VOLUME(a->audio_buffer[a->sample_number + 1], nr50 & 0x07);

#ifdef FLOAT32_AUDIO
            NORMALIZE_SAMPLE(a->audio_buffer[a->sample_number]);
            NORMALIZE_SAMPLE(a->audio_buffer[a->sample_number + 1]);
#endif // FLOAT32_AUDIO
            
            a->sample_number += 2;
            if(a->sample_number == AUDIO_BUFFER_LENGTH){
                SDL_QueueAudio(1, a->audio_buffer, AUDIO_BUFFER_LENGTH * sizeof(AudioSample));
                a->sample_number = 0;
            }
        }
    }
}

static void Update_Ch1(APU *a, int cycles){
    /***** Incomplete *****/
    AudioSample value;
    int frequency, wave_duty;
    BYTE nr11, nr12, nr13, nr14;

    nr11 = Mem_ReadByte(a->memory, NR11_ADDR); // Sound length + Wave duty
    nr12 = Mem_ReadByte(a->memory, NR12_ADDR); // Volume envelope
    nr13 = Mem_ReadByte(a->memory, NR13_ADDR); // Frequency lo
    nr14 = Mem_ReadByte(a->memory, NR14_ADDR); // Initial + Counter/Continuous + Frequency hi

    frequency = nr13;
    frequency |= (nr14 & 0x7) << 8;
    a->frame_countdown[0] = (2048 - frequency) * 4;

    wave_duty = nr11 >> 6;

    // Check Initial flag
    if(TEST_BIT(nr14, 7)){
        Mem_WriteByte(a->memory, NR14_ADDR, nr14 & 0x7F); // Clear initial flag in memory
        // Reset timers and sequence
        float t1 = (float) (nr11 & 0x3F);
        // Sound length = (64 - t1)/256 seconds
        a->sound_timer[0] = us_to_cycles(((64.0 - t1) / 256.0) * 1000000.0);
        a->frame_timer[0] = a->frame_countdown[1];
        a->sequence[0] = 0;
        // Reset evelope value and timer
        float n = (float) (nr12 & 0x07);
        a->envelope_enable[0] = (n == 0) ? false : true;
        a->envelope_value[0] = nr12 >> 4;
        // Length of 1 step = n/64 seconds
        a->envelope_timer[0] = us_to_cycles((n / 64.0) / 1000000.0);
    }

    if(a->sound_timer[0] > 0 || !TEST_BIT(nr14, 6)){
        a->sound_timer[0] -= cycles;
        a->frame_timer[0] -= cycles;
        if(a->frame_timer[0] <= 0){
            a->frame_timer[0] = a->frame_countdown[1];
            a->sequence[0] = (a->sequence[0] + 1) % 8;
        }

        value = square_wave_table[wave_duty][a->sequence[0]];
        a->audio_buffer[a->sample_number] = value;
        a->audio_buffer[a->sample_number + 1] = value;
    }
}

static void Update_Ch2(APU *a, int cycles){
    AudioSample value;
    int frequency, wave_duty;
    BYTE nr21, nr22, nr23, nr24;

    nr21 = Mem_ReadByte(a->memory, NR21_ADDR); // Sound length + Wave duty
    nr22 = Mem_ReadByte(a->memory, NR22_ADDR); // Volume envelope
    nr23 = Mem_ReadByte(a->memory, NR23_ADDR); // Frequency lo
    nr24 = Mem_ReadByte(a->memory, NR24_ADDR); // Initial + Counter/Continuous + Frequency hi

    frequency = nr23;
    frequency |= (nr24 & 0x7) << 8;
    // Frequency = 131072/(2048-x) Hz
    a->frame_countdown[1] = (2048 - frequency) * 4;

    wave_duty = nr21 >> 6;

    // Check Initial flag
    if(TEST_BIT(nr24, 7)){
        Mem_WriteByte(a->memory, NR24_ADDR, nr24 & 0x7F); // Clear initial flag in memory
        // Reset timers and sequence
        float t1 = (float) (nr21 & 0x3F);
        // Sound length = (64 - t1)/256 seconds
        a->sound_timer[1] = us_to_cycles(((64.0 - t1) / 256.0) * 1000000.0);
        a->frame_timer[1] = a->frame_countdown[1];
        a->sequence[1] = 0;
        // Reset evelope value and timer
        float n = (float) (nr22 & 0x07);
        a->envelope_enable[1] = (n == 0) ? false : true;
        a->envelope_value[1] = nr22 >> 4;
        // Length of 1 step = n/64 seconds
        a->envelope_timer[1] = us_to_cycles((n / 64.0) / 1000000.0);
    }

    // Only process sound information if the sound timer hasn't expired or continuous output is enabled
    if(a->sound_timer[1] > 0 || !TEST_BIT(nr24, 6)){
        a->sound_timer[1] -= cycles;
        // Figure out location in the 8-step sequence (from square_wave_table)
        a->frame_timer[1] -= cycles;
        if(a->frame_timer[1] <= 0){
            a->frame_timer[1] = a->frame_countdown[1];
            a->sequence[1] = (a->sequence[1] + 1) % 8;
            a->envelope_value[1] += (TEST_BIT(nr22, 1)) ? 255 : -255;
            a->envelope_value[1] &= 0xFF;
        }

        // Adjust volume if the envelope timer has run out
        a->envelope_timer[1] -= cycles;
        if(a->envelope_enable[1] && a->envelope_timer[1] <= 0){
            float n = (float) (nr22 & 0x07);
            if(n == 0)
                a->envelope_enable[1] = false;
            a->envelope_timer[1] = us_to_cycles((n / 64.0) / 1000000.0);

        }

        value = (square_wave_table[wave_duty][a->sequence[1]] * a->envelope_value[1]) / 0xFFFF;

        if(TEST_BIT(a->nr51, 1))
            a->audio_buffer[a->sample_number] += value;
        if(TEST_BIT(a->nr51, 5))
            a->audio_buffer[a->sample_number + 1] += value;
    }
}

static void Update_Ch3(APU *a, int cycles){

}

static void Update_Ch4(APU *a, int cycles){

}
