#pragma once

#define NES_CREATED_MAGIC_DWORD  0x12345678

typedef struct NES
{
    CPU cpu;

    MAPPER mapper;
    uint8_t cycle_alignment;

    uint8_t* PRG_ROM_data;
    uint32_t PRG_ROM_size;

    uint8_t* CHR_ROM_data;
    uint32_t CHR_ROM_size;

    uint32_t created;   // To check if the nes has been initialized
} NES;

NES nes_create();
void nes_destroy();

void nes_reset(NES* nes);
void nes_power_up(NES* nes);
void nes_load_game(NES* nes, char* path_to_rom);