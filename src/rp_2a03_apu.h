#pragma once

#define APU_SAMPLE_RATE 44100
#define APU_BUFFER_SIZE (APU_SAMPLE_RATE / 30)
#define APU_NUM_BUFFERS 4

#define APU_PULSE_WAVE_HARMONICS        16
#define APU_TRIANGLE_WAVE_HARMONICS     6

#define APU_CHANNEL_PULSE1      0
#define APU_CHANNEL_PULSE2      1
#define APU_CHANNEL_TRIANGLE    2
#define APU_CHANNEL_NOISE       3
#define APU_CHANNEL_DMC         4

#define APU_VOLUME              0.5

typedef struct RP_2A03_APU
{
    HWAVEOUT wave_out;
    WAVEFORMATEX wfx;
    WAVEHDR wave_headers[APU_NUM_BUFFERS];
    uint8_t buffers[APU_NUM_BUFFERS][APU_BUFFER_SIZE];
    int current_buffer;

    float pulse1_time;
    float pulse1_duty_cycle;
    float pulse1_frequency;
} APU;

static void CALLBACK apu_wave_out_callback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
static void apu_fill_buffer(APU *apu, uint8_t *buffer, size_t size);