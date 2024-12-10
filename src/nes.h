#pragma once

#define NES_CREATED_MAGIC_DWORD  0x12345678

typedef struct NES
{
    CPU cpu;
    PPU ppu;

    MAPPER mapper;      // Mapper used by currently loaded game
    uint8_t cycle_alignment;    // Alignment between the cpu and ppu

    uint8_t* PRG_ROM_data;
    uint8_t* CHR_ROM_data;
    uint32_t PRG_ROM_size;
    uint32_t CHR_ROM_size;
    uint8_t* PRG_RAM;
    bool CHR_RAM;

    uint8_t selected_prgrom_bank_0;
    // uint8_t selected_prgrom_bank_1;
    uint8_t selected_prgram_bank;
    uint8_t selected_chrrom_bank_0;
    uint8_t selected_chrrom_bank_1;

    uint8_t mmc1_shift_register;
    uint8_t mmc1_bits_shifted;
    uint8_t mmc1_control, mmc1_chr_bank_0, mmc1_chr_bank_1, mmc1_prg_bank;

    uint8_t key_status;
    bool key_strobe;

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