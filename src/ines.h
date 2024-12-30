#pragma once

typedef enum MAPPER
{
    MP_NROM = 0,
    MP_MMC1 = 1,
    MP_UxROM = 2,
    MP_AxROM = 7,
    MP_UNSUPPORTED = 0xffff,
} MAPPER;

typedef enum TV_SYSTEM
{
    TS_NTSC = 0,
    TS_PAL = 1
} TV_SYSTEM;

char* system_text[2] = 
{
    "NTSC",
    "PAL"
};

struct iNES_FLAGS_6
{
    uint8_t nametable_layout : 1;       // 0: vertical arrangement ("horizontal mirrored") (CIRAM A10 = PPU A11)
                                        // 1: horizontal arrangement ("vertically mirrored") (CIRAM A10 = PPU A10)
    uint8_t persistent_memory : 1;  // 1: Cartridge contains battery-backed PRG RAM ($6000-7FFF) or other persistent memory
    uint8_t trainer : 1;            // 1: 512-byte trainer at $7000-$71FF (stored before PRG data)
    uint8_t other_nametable_layout : 1; // 1: Alternative nametable layout
    uint8_t mapper_lo : 4;          // Lower nybble of mapper number
} __attribute__((packed));

struct iNES_FLAGS_7
{
    uint8_t vs_unisystem : 1;       // VS Unisystem
    uint8_t pc_10 : 1;              // PlayChoice-10 (8 KB of Hint Screen data stored after CHR data)
    uint8_t NES_20 : 2;            // If equal to 2, flags 8-15 are in NES 2.0 format
    uint8_t mapper_hi : 4;          // Upper nybble of mapper number
} __attribute__((packed));

struct iNES_FLAGS_8
{
    uint8_t prg_ram_size;       // PRG RAM size in 8KB units (0 infers 8KB for compability)
} __attribute__((packed));

struct iNES_FLAGS_9
{
    uint8_t tv_system;       // 0: NTSC ; 1: PAL
} __attribute__((packed));

struct iNES_HEADER
{
    uint8_t NES[4];     // $4E $45 $53 $1A
    uint8_t prg_rom;    // Size of PRG ROM in 16KB units
    uint8_t chr_rom;    // Size of CHR ROM in 8KB units (value 0 means the board uses CHR RAM)
    struct iNES_FLAGS_6 flags_6;
    struct iNES_FLAGS_7 flags_7;
    struct iNES_FLAGS_8 flags_8;
    struct iNES_FLAGS_9 flags_9;
    uint8_t flags_10;

    uint8_t padding[5];
} __attribute__((packed));