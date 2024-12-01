#pragma once

typedef struct NES
{
    CPU cpu;

    MAPPER mapper;
    uint8_t cycle_alignment;
} NES;

typedef enum MAPPER
{
    MP_NROM = 0,
    MP_UNSUPPORTED = 0xffff,
} MAPPER;

void nes_reset(NES* nes);
void nes_power_up(NES* nes);
void nes_load_game(NES* nes, char* path_to_rom);