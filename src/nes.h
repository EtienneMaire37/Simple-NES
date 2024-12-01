#pragma once

typedef struct NES
{
    CPU cpu;

    uint8_t cycle_alignment;
} NES;

void nes_reset(NES* nes);
void nes_power_up(NES* nes);