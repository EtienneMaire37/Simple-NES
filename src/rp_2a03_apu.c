#pragma once

void apu_reset(APU* apu)
{
    apu->pulse1_duty_cycle = 0.5;
    apu->pulse1_frequency = 0;
    apu->pulse1_timer_period = 0;
    apu->pulse1_length_counter = 0;
    apu->pulse1_lc_halt = 0;
    apu->pulse1_volume = 0;
    apu->pulse1_constant_volume = 1;
    apu->pulse1_start_flag = 0;
    apu->pulse1_envelope_divider = 0;

    apu->irq_inhibit = apu->sequencer_mode = 0;
    apu->cpu_cycles = 0;
    *(uint8_t*)&apu->status = 0;
}

void apu_half_frame(APU* apu)
{
    if (apu->pulse1_length_counter != 0 || apu->pulse1_lc_halt)
        apu->pulse1_length_counter--;
}

void apu_quarter_frame(APU* apu)
{
    if (apu->pulse1_envelope_divider == 0)
    {
        apu->pulse1_envelope_divider = apu->pulse1_volume;
        if (apu->pulse1_decay_volume == 0)
        {
            if (apu->pulse1_loop)
                apu->pulse1_decay_volume = 15;
        }
        else
            apu->pulse1_decay_volume--;
    }
    else
        apu->pulse1_envelope_divider--;
    if (apu->pulse1_start_flag)
    {
        apu->pulse1_decay_volume = 15;
        apu->pulse1_envelope_divider = apu->pulse1_volume;
    }
}

void apu_cycle(APU* apu)
{
    if (apu->sequencer_mode)
    {
        if (apu->cpu_cycles == 7456.5 * 2 || apu->cpu_cycles == 18640.5 * 2)
            apu_half_frame(apu);

        if (apu->cpu_cycles == 3728.5 * 2 || apu->cpu_cycles == 7456.5 * 2 ||
            apu->cpu_cycles == 11185.5 * 2 || apu->cpu_cycles == 18640.5 * 2)
            apu_quarter_frame(apu);
    }
    else
    {
        if (apu->cpu_cycles == 7456.5 * 2 || apu->cpu_cycles == 14914.5 * 2)
            apu_half_frame(apu);

        if (apu->cpu_cycles == 3728.5 * 2 || apu->cpu_cycles == 7456.5 * 2 ||
            apu->cpu_cycles == 11185.5 * 2 || apu->cpu_cycles == 14914.5 * 2)
            apu_quarter_frame(apu);
    }
}

void apu_init(APU* apu) 
{
#ifdef ENABLE_AUDIO
    apu->wave_out = 0;
    apu->pulse1_time = 0;
    apu->current_buffer = 0;

    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, APU_SAMPLE_RATE, APU_SAMPLE_RATE, 1, 8, 0 };
    apu->wfx = wfx;

    if (waveOutOpen(&apu->wave_out, WAVE_MAPPER, &apu->wfx, (DWORD_PTR)apu_wave_out_callback, (DWORD_PTR)apu, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) 
    {
        printf("Failed to open audio device\n");
        return;
    }

    for (uint8_t i = 0; i < APU_NUM_BUFFERS; i++) 
    {
        for (uint32_t j = 0; j < APU_BUFFER_SIZE; j++)
            apu->buffers[i][j] = 0;

        WAVEHDR* hdr = &apu->wave_headers[i];
        hdr->lpData = (LPSTR)apu->buffers[i];
        hdr->dwBufferLength = APU_BUFFER_SIZE;
        hdr->dwFlags = 0;
        hdr->dwLoops = 0;

        if (waveOutPrepareHeader(apu->wave_out, hdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) 
        {
            printf("Failed to prepare audio buffer %d\n", i);
            return;
        }

        if (waveOutWrite(apu->wave_out, hdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) 
        {
            printf("Failed to write audio buffer %d\n", i);
            return;
        }
    }
#endif
}

float apu_getchannel(APU* apu, uint8_t channel)
{
    if (!(channel >= 0 && channel < 5))
        return 0;

    switch (channel)
    {
    case APU_CHANNEL_PULSE1:
    {
        if (apu->pulse1_timer_period < 8 || apu->pulse1_length_counter == 0 || !apu->status.pulse_1)
            return 0;
        float amplitude = 0.5;
        float omega = 2 * M_PI * apu->pulse1_frequency;
        float val = 0;
        apu->pulse1_time += omega / (float)APU_SAMPLE_RATE;
        for (uint8_t i = 1; i <= APU_PULSE_WAVE_HARMONICS; i++)
            val += sin(M_PI * i * apu->pulse1_duty_cycle) * cos(i * apu->pulse1_time) / i;
        val *= 2 * amplitude / M_PI;
        val += amplitude * apu->pulse1_duty_cycle + 0.125;
        return val * (apu->pulse1_constant_volume ? apu->pulse1_volume : apu->pulse1_decay_volume);
    }

    default:
        return 0;
    }
}

float apu_pulse_out(APU* apu)
{
    float sum = apu_getchannel(apu, 0) + apu_getchannel(apu, 1);
    if (sum == 0)
        return 0;
    float out = 95.88f / ((8128 / sum) + 100);
    return max(0, min(1, out));
}

#ifdef ENABLE_AUDIO
static void apu_fill_buffer(APU* apu, uint8_t* buffer, uint32_t size) 
{
    for (uint32_t i = 0; i < size; i++) 
    {
        if (emulation_running)
        {
            float val = apu_pulse_out(apu);
            buffer[i] = (uint8_t)(val * APU_VOLUME * 255);
        }
        else
            buffer[i] = 0;
    }
}

static void CALLBACK apu_wave_out_callback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) 
{
    if (uMsg == WOM_DONE) 
    {
        APU* apu = (APU*)dwInstance;

        WAVEHDR* hdr = &apu->wave_headers[apu->current_buffer];
        apu_fill_buffer(apu, (uint8_t*)hdr->lpData, hdr->dwBufferLength);

        if (waveOutWrite(apu->wave_out, hdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) 
        {
            printf("Failed to write audio buffer\n");
        }

        apu->current_buffer = (apu->current_buffer + 1) % APU_NUM_BUFFERS;
    }
}
#endif

void apu_destroy(APU* apu) 
{
    #ifdef ENABLE_AUDIO
    for (int i = 0; i < APU_NUM_BUFFERS; i++) 
        waveOutUnprepareHeader(apu->wave_out, &apu->wave_headers[i], sizeof(WAVEHDR));

    waveOutClose(apu->wave_out);
    #endif
}
