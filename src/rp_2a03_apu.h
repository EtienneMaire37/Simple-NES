#pragma once

#define APU_SAMPLE_RATE 44100
#define APU_BUFFER_SIZE (APU_SAMPLE_RATE / 60)
#define APU_NUM_BUFFERS 8

#define APU_CHANNEL_PULSE1      0
#define APU_CHANNEL_PULSE2      1
#define APU_CHANNEL_TRIANGLE    2
#define APU_CHANNEL_NOISE       3
#define APU_CHANNEL_DMC         4

#define APU_VOLUME              0.5

#define loop                    lc_halt

struct APU_STATUS
{
    uint8_t pulse_1 : 1;
    uint8_t pulse_2 : 1;
    uint8_t triangle : 1;
    uint8_t noise : 1;
    uint8_t dmc : 1;

    uint8_t padding : 3;
} __attribute__((packed));

typedef struct APU_PULSE_CHANNEL
{
    uint8_t selected_duty;
    uint16_t timer_period;
    uint16_t timer;
    uint16_t length_counter;
    bool lc_halt;
    uint8_t volume;
    uint8_t decay_volume;
    bool start_flag;
    uint8_t envelope_divider;
    bool constant_volume;
    bool sweep_enabled;
    uint16_t target_period;
    uint8_t sweep_period;
    uint8_t sweep_divider;
    uint8_t sweep_shift;
    bool sweep_reload;
    bool sweep_negate;
    uint8_t sequencer;
} APU_PULSE_CHANNEL;

typedef struct RP_2A03_APU
{
#ifdef ENABLE_AUDIO
    HWAVEOUT wave_out;
    WAVEFORMATEX wfx;
    WAVEHDR wave_headers[APU_NUM_BUFFERS];
    uint16_t buffers[APU_NUM_BUFFERS][APU_BUFFER_SIZE];
    int current_buffer;
#endif

    bool sequencer_mode;    // 0 : 4-step sequence, 1 : 5-step sequence
    bool irq_inhibit;
    struct APU_STATUS status;
    uint64_t samples;
    uint64_t total_cycles;
    uint64_t cpu_cycles;

    APU_PULSE_CHANNEL pulse1;
    APU_PULSE_CHANNEL pulse2;

    NES* nes;
} APU;

uint8_t apu_length_counter_lookup[32] = 
{
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

uint8_t pulse_duty_cycles[4] = 
{
    0b01000000, 0b01100000, 0b01111000, 0b10011111
};

void apu_reset(APU* apu);
void apu_init_pulse_channel(APU_PULSE_CHANNEL* channel);
void apu_pulse_channel_quarter_frame(APU_PULSE_CHANNEL* channel);
void apu_pulse_channel_half_frame(APU* apu, APU_PULSE_CHANNEL* channel);
void apu_pulse_channel_cycle(APU* apu, APU_PULSE_CHANNEL* channel);
void apu_half_frame(APU* apu);
void apu_quarter_frame(APU* apu);
void apu_cycle(APU* apu);
void apu_pulse_channel_register_0_write(APU_PULSE_CHANNEL* channel, uint8_t value);
void apu_pulse_channel_register_1_write(APU_PULSE_CHANNEL* channel, uint8_t value);
void apu_pulse_channel_register_2_write(APU* apu, APU_PULSE_CHANNEL* channel, uint8_t value);
void apu_pulse_channel_register_3_write(APU* apu, APU_PULSE_CHANNEL* channel, uint8_t value);
void apu_init(APU* apu);
float apu_get_pulse_channel_output(APU* apu, APU_PULSE_CHANNEL* channel, bool status);
float apu_getchannel(APU* apu, uint8_t channel);
float apu_pulse_out(APU* apu);
void apu_destroy(APU* apu);

#ifdef ENABLE_AUDIO
static void CALLBACK apu_wave_out_callback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
static void apu_fill_buffer(APU* apu, uint8_t* buffer, uint32_t size);
#endif
