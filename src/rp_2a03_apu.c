#pragma once

void apu_reset(APU* apu)
{
    apu_init_pulse_channel(apu->nes, &apu->pulse1);
    apu_init_pulse_channel(apu->nes, &apu->pulse2);

    apu->irq_inhibit = apu->sequencer_mode = 0;
    apu->cpu_cycles = 0;
    *(uint8_t*)&apu->status = 0;
    apu->samples = 0;
    apu->total_cycles = 0;
}

void apu_init_pulse_channel(NES* nes, APU_PULSE_CHANNEL* channel)
{
    channel->selected_duty = 0;
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
    channel->sequencer = channel->smooth_sequencer = 0b11110000;
    apu_pulse_channel_update_smooth_timer(nes, channel);
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
    channel->target_period = maxint(0, channel->timer_period + change_amount);
}

void apu_half_frame(APU* apu)
{
    // if (apu->status.pulse_1)
        apu_pulse_channel_half_frame(apu, &apu->pulse1);
    // if (apu->status.pulse_2)
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
        // apu_pulse_channel_update_smooth_timer(channel);
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

    if (apu->sequencer_mode)    // 5 step sequence
    {
        if (apu->nes->system == TV_NTSC)
        {
            if (apu->cpu_cycles == 7456.5 * 2 || apu->cpu_cycles == 18640.5 * 2)
                apu_half_frame(apu);

            if (apu->cpu_cycles == 3728.5 * 2 || apu->cpu_cycles == 7456.5 * 2 ||
                apu->cpu_cycles == 11185.5 * 2 || apu->cpu_cycles == 18640.5 * 2)
                apu_quarter_frame(apu);
        }
        else
        {
            if (apu->cpu_cycles == 8313.5 * 2 || apu->cpu_cycles == 20782.5 * 2)
                apu_half_frame(apu);

            if (apu->cpu_cycles == 4156.5 * 2 || apu->cpu_cycles == 8313.5 * 2 ||
                apu->cpu_cycles == 12469.5 * 2 || apu->cpu_cycles == 20782.5 * 2)
                apu_quarter_frame(apu);
        }
    }
    else
    {
        if (apu->nes->system == TV_NTSC)
        {
            if (apu->cpu_cycles == 7456.5 * 2 || apu->cpu_cycles == 14914.5 * 2)
                apu_half_frame(apu);

            if (apu->cpu_cycles == 3728.5 * 2 || apu->cpu_cycles == 7456.5 * 2 ||
                apu->cpu_cycles == 11185.5 * 2 || apu->cpu_cycles == 14914.5 * 2)
                apu_quarter_frame(apu);
        }
        else
        {

        }
    }
}

void apu_pulse_channel_register_0_write(NES* nes, APU_PULSE_CHANNEL* channel, uint8_t value)
{
    channel->selected_duty = (value >> 6);
    channel->sequencer = pulse_duty_cycles[channel->selected_duty];
    channel->smooth_sequencer = channel->sequencer;
    channel->lc_halt = (value >> 5) & 1;
    channel->constant_volume = (value >> 4) & 1;
    channel->volume = (value & 0b1111);
    apu_pulse_channel_update_smooth_timer(nes, channel);
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
}

void apu_pulse_channel_register_3_write(APU* apu, APU_PULSE_CHANNEL* channel, uint8_t value)
{
    channel->timer_period &= 0x00ff;
    channel->timer_period |= ((uint16_t)value & 0b111) << 8;
    channel->timer_period &= 0b11111111111;
    channel->start_flag = 1;
    channel->timer = channel->timer_period;
    channel->sequencer = pulse_duty_cycles[channel->selected_duty];
    channel->smooth_sequencer = channel->sequencer;
    apu_pulse_channel_update_smooth_timer(apu->nes, channel);
}

void apu_init(APU* apu) 
{
#ifdef ENABLE_AUDIO
    apu->stream = NULL;
    apu->current_buffer = 0;

    printf("Creating sound stream...\n");

    apu->stream = sfSoundStream_create(apu_sound_get_data, apu_sound_seek, 1, APU_SAMPLE_RATE, apu);
    if (!apu->stream)
    {
        printf("Failed to create sound stream\n");
        return;
    }

    printf("Created sound stream\n");

    sfSoundStream_play(apu->stream);
#endif
    audio_initialised = true;
}

float apu_get_pulse_channel_output(APU* apu, APU_PULSE_CHANNEL* channel, bool status)
{
    if (channel->timer_period < 8 || channel->length_counter == 0 || !status || channel->target_period > 0x7ff)
        return 0;
    return (channel->sequencer >> 7) * (channel->constant_volume ? channel->volume : channel->decay_volume);
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
    return fmaxf(0, fminf(1, out));
}

void apu_pulse_channel_update_smooth_timer(NES* nes, APU_PULSE_CHANNEL* channel)
{
    channel->smooth_timer = (16 * (channel->timer_period + 1)) / (double)((nes->system == TV_NTSC ? NTSC_CPU_FREQUENCY : PAL_CPU_FREQUENCY) * 6);
}

void apu_pulse_channel_handle_smooth_sequencing(NES* nes, APU_PULSE_CHANNEL* channel, double delta_time)
{
    if (channel->smooth_timer <= 0)
    {
        channel->smooth_sequencer = (channel->smooth_sequencer << 1) | (channel->smooth_sequencer >> 7);
        apu_pulse_channel_update_smooth_timer(nes, channel);
    }
    else
        channel->smooth_timer -= delta_time;
}

#ifdef ENABLE_AUDIO
void apu_fill_buffer(APU* apu, sfInt16* buffer, uint32_t size) 
{
    // if (((int)apu->nes->sound_buffer_out - apu->nes->actual_sound_buffer_in) % ((int)PAL_MASTER_FREQUENCY + 1) > (PAL_MASTER_FREQUENCY / 60))
    //     apu->nes->sound_buffer_out = apu->nes->actual_sound_buffer_in - frame_cycles(apu->nes->system);
    while (apu->nes->sound_buffer_out >= PAL_MASTER_FREQUENCY + 1)
        apu->nes->sound_buffer_out -= PAL_MASTER_FREQUENCY + 1;
    for (int i = 0; i < size; i++)
    {
        float val = apu->nes->sound_buffer[(int)apu->nes->sound_buffer_out]; // 0..1
        apu->nes->sound_buffer_out += frame_cycles(apu->nes->system) / size;
        // if ((apu->nes->sound_buffer_out - apu->nes->actual_sound_buffer_in) % ((int)PAL_MASTER_FREQUENCY + 1) > (PAL_MASTER_FREQUENCY / 60))
        //     apu->nes->sound_buffer_out = apu->nes->actual_sound_buffer_in - frame_cycles(apu->nes->system) / size * 120;
        while (apu->nes->sound_buffer_out >= PAL_MASTER_FREQUENCY + 1)
            apu->nes->sound_buffer_out -= PAL_MASTER_FREQUENCY + 1;
        buffer[i] = (sfInt16)((val * 65535 - 32768) * APU_VOLUME);
    }
}
#endif

#ifdef ENABLE_AUDIO
sfBool apu_sound_get_data(sfSoundStreamChunk* chunk, void* instance)
{
    APU* apu = (APU*)instance;

    bool audio_not_initialised = false;
    while (!audio_initialised)
    {
        audio_not_initialised = true;
        if (audio_destroyed)
            return sfFalse;
    }

    if (audio_not_initialised)
        return sfFalse;

    sfInt16* buf = apu->buffers[apu->current_buffer];
    apu_fill_buffer(apu, buf, APU_BUFFER_SIZE);

    chunk->samples = buf;
    chunk->sampleCount = (sfUint64)APU_BUFFER_SIZE;

    apu->current_buffer = (apu->current_buffer + 1) % APU_NUM_BUFFERS;

    return sfTrue;
}

void apu_sound_seek(sfTime timeOffset, void* instance)
{
    (void)instance;
    (void)timeOffset;
}
#endif

void apu_destroy(APU* apu) 
{
    #ifdef ENABLE_AUDIO
    audio_initialised = false;
    audio_destroyed = true;

    if (apu->stream)
    {
        sfSoundStream_stop(apu->stream);
        sfSoundStream_destroy(apu->stream);
        apu->stream = NULL;
    }
    #endif
}
