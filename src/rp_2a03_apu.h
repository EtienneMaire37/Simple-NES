#pragma once

#define APU_SAMPLE_RATE 44100
#define APU_BUFFER_SIZE (APU_SAMPLE_RATE / 30)
#define APU_NUM_BUFFERS 4

#define APU_PULSE_WAVE_HARMONICS        12
#define APU_TRIANGLE_WAVE_HARMONICS     6

#define APU_CHANNEL_PULSE1      0
#define APU_CHANNEL_PULSE2      1
#define APU_CHANNEL_TRIANGLE    2
#define APU_CHANNEL_NOISE       3
#define APU_CHANNEL_DMC         4

#define APU_VOLUME              0.5

struct APU_STATUS
{
    uint8_t pulse_1 : 1;
    uint8_t pulse_2 : 1;
    uint8_t triangle : 1;
    uint8_t noise : 1;
    uint8_t dmc : 1;

    uint8_t padding : 3;
} __attribute__((packed));

typedef struct RP_2A03_APU
{
    HWAVEOUT wave_out;
    WAVEFORMATEX wfx;
    WAVEHDR wave_headers[APU_NUM_BUFFERS];
    uint8_t buffers[APU_NUM_BUFFERS][APU_BUFFER_SIZE];
    int current_buffer;

    bool sequencer_mode;    // 0 : 4-step sequence, 1 : 5-step sequence
    bool irq_inhibit;
    struct APU_STATUS status;

    uint32_t cpu_cycles;

    float pulse1_time;
    float pulse1_duty_cycle;
    float pulse1_frequency;
    uint16_t pulse1_timer_period;
    uint16_t pulse1_length_counter;
} APU;

uint8_t apu_length_counter_lookup[32] = 
{
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static void CALLBACK apu_wave_out_callback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
static void apu_fill_buffer(APU *apu, uint8_t *buffer, size_t size);