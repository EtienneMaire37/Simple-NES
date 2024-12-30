#pragma once

#define NES_CREATED_MAGIC_DWORD  0x12345678

#define NTSC_MASTER_FREQUENCY       (236250000 / 11.f) // Hz
#define NTSC_CPU_FREQUENCY          1789773 // Hz

#define PAL_MASTER_FREQUENCY        26601712.5 // Hz
#define PAL_CPU_FREQUENCY           (26600000 / 16.f) // Hz

typedef struct NES
{
    CPU cpu;
    APU apu;
    PPU ppu;

    MAPPER mapper;      // Mapper used by currently loaded game
    uint64_t master_clock;

    uint8_t* PRG_ROM_data;
    uint8_t* CHR_ROM_data;
    uint32_t PRG_ROM_size;
    uint32_t CHR_ROM_size;
    uint8_t* PRG_RAM_data;
    uint32_t PRG_RAM_size;
    bool CHR_RAM;

    uint8_t selected_prgrom_bank_0;
    // uint8_t selected_prgrom_bank_1;
    uint8_t selected_prgram_bank;
    uint8_t selected_chrrom_bank_0;
    uint8_t selected_chrrom_bank_1;

    uint8_t mmc1_shift_register;
    uint8_t mmc1_bits_shifted;
    uint8_t mmc1_control;

    uint8_t key_status;
    uint8_t key_status_control;
    bool key_strobe;

    TV_SYSTEM system;

    uint32_t created;   // To check if the nes has been initialized
} NES;

sfKeyCode keymap[8] = 
{
    sfKeyC,     // A
    sfKeyX,     // B
    sfKeyLControl, // Select
    sfKeyLShift,   // Start
    sfKeyUp,     // Up
    sfKeyDown,   // Down
    sfKeyLeft,   // Left
    sfKeyRight   // Right
};

NES nes_create();
void nes_destroy();

void nes_reset(NES* nes);
void nes_power_up(NES* nes);
void nes_load_game(NES* nes, char* path_to_rom);