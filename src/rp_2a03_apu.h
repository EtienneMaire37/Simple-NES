#pragma once

#define APU_SAMPLE_RATE 44100
#define APU_BUFFER_SIZE (APU_SAMPLE_RATE / 30)
#define APU_NUM_BUFFERS 4

typedef struct RP_2A03_APU
{
    HWAVEOUT wave_out;
    WAVEFORMATEX wfx;
    WAVEHDR wave_headers[APU_NUM_BUFFERS];
    uint8_t buffers[APU_NUM_BUFFERS][APU_BUFFER_SIZE];
    float time;
    int current_buffer;
} APU;

static void CALLBACK wave_out_callback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
static void fill_buffer(APU *apu, uint8_t *buffer, size_t size);