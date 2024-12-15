#pragma once

void apu_reset(APU* apu)
{
    apu_init_pulse_channel(&apu->pulse1);
    apu_init_pulse_channel(&apu->pulse2);

    apu->irq_inhibit = apu->sequencer_mode = 0;
    apu->cpu_cycles = 0;
    *(uint8_t*)&apu->status = 0;
}

void apu_init_pulse_channel(APU_PULSE_CHANNEL* channel)
{
    channel->duty_cycle = 0.5;
    channel->frequency = 0;
    channel->timer_period = 0;
    channel->length_counter = 0;
    channel->lc_halt = 0;
    channel->volume = 0;
    channel->constant_volume = 1;
    channel->start_flag = 0;
    channel->envelope_divider = 0;
    channel->sweep_enabled = 0;
    channel->sweep_period = 0;
    channel->sweep_negate = 0;
    channel->sweep_shift = 0;
    channel->target_period = 0;
    channel->sweep_divider = 0;
    channel->sweep_reload = 0;
    channel->time = 0;
    channel->sequencer = 0b11110000;
}

void apu_reload_pulse_frequency(APU_PULSE_CHANNEL* channel)
{
    channel->frequency = CPU_FREQUENCY / (16 * (channel->timer_period + 1));
}

void apu_pulse_channel_quarter_frame(APU_PULSE_CHANNEL* channel)
{
    if (channel->start_flag)
    {
        channel->decay_volume = 15;
        channel->envelope_divider = channel->volume;
        channel->start_flag = 0;
    }
    else
    {
        channel->envelope_divider--;
    }

    if (channel->envelope_divider == 0)
    {
        channel->envelope_divider = channel->volume;
        if (channel->decay_volume == 0)
        {
            if (channel->loop)
                channel->decay_volume = 15;
        }
        else
            channel->decay_volume--;
    }
}

void apu_pulse_channel_half_frame(APU* apu, APU_PULSE_CHANNEL* channel)
{
    if (channel->length_counter != 0 && !channel->lc_halt)
        channel->length_counter--;

    if (channel->sweep_divider == 0 && channel->sweep_shift != 0 && channel->sweep_enabled)
    {
        if (!(channel->timer_period < 8 || channel->target_period > 0x7ff))
        {
            channel->timer_period = channel->target_period;
            apu_reload_pulse_frequency(channel);
        }
    }

    if (channel->sweep_shift != 0)
    {
        if (channel->sweep_reload || channel->sweep_divider == 0)
        {
            channel->sweep_divider = channel->sweep_period;
            channel->sweep_reload = 0;
        }
        else
            channel->sweep_divider--;
    }
}

void apu_pulse_channel_cycle(APU* apu, APU_PULSE_CHANNEL* channel)
{
    int16_t change_amount = (channel->timer_period >> channel->sweep_shift);
    if (channel->sweep_negate)
        change_amount = -change_amount - 1 + (channel == &apu->pulse2 ? 1 : 0);     // One's complement or Two's complement
    channel->target_period = max(0, channel->timer_period + change_amount);
}

void apu_half_frame(APU* apu)
{
    apu_pulse_channel_half_frame(apu, &apu->pulse1);
    apu_pulse_channel_half_frame(apu, &apu->pulse2);
}

void apu_quarter_frame(APU* apu)
{
    apu_pulse_channel_quarter_frame(&apu->pulse1);
    apu_pulse_channel_quarter_frame(&apu->pulse2);
}

void apu_pulse_cycle(APU_PULSE_CHANNEL* channel)
{
    if (channel->timer == 0)
    {
        channel->timer = channel->timer_period;
        channel->sequencer = (channel->sequencer << 1) | (channel->sequencer >> 7);
    }
    else
        channel->timer--;
}

void apu_cycle(APU* apu)
{
    apu_pulse_channel_cycle(apu, &apu->pulse1);
    apu_pulse_channel_cycle(apu, &apu->pulse2);

    if (apu->cpu_cycles % 2 == 0)
    {
        apu_pulse_cycle(&apu->pulse1);
        apu_pulse_cycle(&apu->pulse2);
    }

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

void apu_pulse_channel_register_0_write(APU_PULSE_CHANNEL* channel, uint8_t value)
{
    switch (value >> 6)
    {
    case 0:
        channel->duty_cycle = 0.125;
        channel->sequencer = 0b01000000;
        break;
    case 1:
        channel->duty_cycle = 0.25;
        channel->sequencer = 0b01100000;
        break;
    case 2:
        channel->duty_cycle = 0.50;
        channel->sequencer = 0b01111000;
        break;
    case 3:
        channel->duty_cycle = 0.75;
        channel->sequencer = 0b10011111;
        break;
    }
    channel->lc_halt = (value >> 5) & 1;
    channel->constant_volume = (value >> 4) & 1;
    channel->volume = (value & 0b1111);
}

void apu_pulse_channel_register_1_write(APU_PULSE_CHANNEL* channel, uint8_t value)
{
    channel->sweep_period = ((value >> 4) & 0b111);
    channel->sweep_enabled = (value >> 7);
    channel->sweep_negate = (value >> 3) & 1;
    channel->sweep_shift = (value & 0b111);
    channel->sweep_reload = 1;
}

void apu_pulse_channel_register_2_write(APU* apu, APU_PULSE_CHANNEL* channel, uint8_t value)
{
    channel->timer_period &= 0xff00;
    channel->timer_period |= value;
    apu_reload_pulse_frequency(channel);
}

void apu_pulse_channel_register_3_write(APU* apu, APU_PULSE_CHANNEL* channel, uint8_t value)
{
    channel->timer_period &= 0x00ff;
    channel->timer_period |= ((uint16_t)value & 0b111) << 8;
    channel->timer_period &= 0b11111111111;
    apu_reload_pulse_frequency(channel);
    channel->start_flag = 1;
    channel->timer = channel->timer_period;
}

void apu_init(APU* apu) 
{
#ifdef ENABLE_AUDIO
    apu->wave_out = 0;
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

float apu_get_pulse_channel_output(APU* apu, APU_PULSE_CHANNEL* channel, bool status)
{
    if (emulation_running)
        for (uint32_t i = 0; i < 341 * 262 * 60 / 44100 / 2; i++)
            nes_cycle(apu->nes);
    channel->time += 1 / 44100.f;
    if (channel->timer_period < 8 || channel->length_counter == 0 || !status || channel->target_period > 0x7ff)
        return 0;
    return (channel->sequencer >> 7) * (channel->constant_volume ? channel->volume : channel->decay_volume);

    float amplitude = 0.5;
    float omega = 2 * M_PI * channel->frequency;
    float val = 0;
    channel->time += omega / (float)APU_SAMPLE_RATE;
    for (uint8_t i = 1; i <= APU_PULSE_WAVE_HARMONICS; i++)
        val += sin(M_PI * i * channel->duty_cycle) * cos(i * channel->time) / i;
    val *= 2 * amplitude / M_PI;
    val += amplitude * channel->duty_cycle + 0.125;
    return val * (channel->constant_volume ? channel->volume : channel->decay_volume);
}

float apu_getchannel(APU* apu, uint8_t channel)
{
    if (!(channel >= 0 && channel < 5))
        return 0;

    switch (channel)
    {
    case APU_CHANNEL_PULSE1:
        return apu_get_pulse_channel_output(apu, &apu->pulse1, apu->status.pulse_1);
    case APU_CHANNEL_PULSE2:
        return apu_get_pulse_channel_output(apu, &apu->pulse2, apu->status.pulse_2);

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
