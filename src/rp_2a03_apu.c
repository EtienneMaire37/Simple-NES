#pragma once

void apu_reset(APU *apu)
{
    ;
}

void apu_init(APU *apu) 
{
    apu->wave_out = 0;
    apu->time = 0;
    apu->current_buffer = 0;

    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, APU_SAMPLE_RATE, APU_SAMPLE_RATE, 1, 8, 0 };
    apu->wfx = wfx;

    if (waveOutOpen(&apu->wave_out, WAVE_MAPPER, &apu->wfx, (DWORD_PTR)wave_out_callback, (DWORD_PTR)apu, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) 
    {
        printf("Failed to open audio device.\n");
        return;
    }

    for (int i = 0; i < APU_NUM_BUFFERS; i++) 
    {
        fill_buffer(apu, apu->buffers[i], APU_BUFFER_SIZE);

        WAVEHDR *hdr = &apu->wave_headers[i];
        hdr->lpData = (LPSTR)apu->buffers[i];
        hdr->dwBufferLength = APU_BUFFER_SIZE;
        hdr->dwFlags = 0;
        hdr->dwLoops = 0;

        if (waveOutPrepareHeader(apu->wave_out, hdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) 
        {
            printf("Failed to prepare audio buffer %d.\n", i);
            return;
        }

        if (waveOutWrite(apu->wave_out, hdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) 
        {
            printf("Failed to write audio buffer %d.\n", i);
            return;
        }
    }
}

static void fill_buffer(APU *apu, uint8_t *buffer, size_t size) 
{
    for (size_t i = 0; i < size; i++) {
        float val = sinf(apu->time) * 0.5f + 0.5f;
        apu->time += 2.0f * M_PI * 440.0f / (float)APU_SAMPLE_RATE;
        buffer[i] = (uint8_t)(val * 255);
    }
}

static void CALLBACK wave_out_callback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) 
{
    if (uMsg == WOM_DONE) 
    {
        APU *apu = (APU *)dwInstance;

        WAVEHDR *hdr = &apu->wave_headers[apu->current_buffer];
        fill_buffer(apu, (uint8_t *)hdr->lpData, hdr->dwBufferLength);

        if (waveOutWrite(apu->wave_out, hdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) 
        {
            fprintf(stderr, "Failed to write audio buffer.\n");
        }

        apu->current_buffer = (apu->current_buffer + 1) % APU_NUM_BUFFERS;
    }
}

void apu_destroy(APU *apu) 
{
    waveOutReset(apu->wave_out);

    for (int i = 0; i < APU_NUM_BUFFERS; i++) 
        waveOutUnprepareHeader(apu->wave_out, &apu->wave_headers[i], sizeof(WAVEHDR));

    waveOutClose(apu->wave_out);
}
