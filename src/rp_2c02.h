#pragma once

typedef struct PPU_STATUS
{
    uint8_t open_bus : 5;
    uint8_t sprite_overflow : 1;
    uint8_t sprite_0_hit : 1;
    uint8_t vblank : 1;
} PPU_STATUS;

typedef struct PPU_CONTROL
{
    uint8_t base_nametable_address : 2; // (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
    uint8_t address_increment : 1;      // (0 = add 1 (horizontal); 1 = add 32 (vertical))
    uint8_t sprite_pattern_table_address : 1;       // (0: $0000; 1: $1000; ignored in 8x16 mode)
    uint8_t background_pattern_table_address : 1;   // (0: $0000; 1: $1000)
    uint8_t sprite_size : 1;            // (0: 8x8 pixels; 1: 8x16 pixels)
    uint8_t ppu_master_slave : 1;       // (0: read backdrop from EXT pins; 1: output color on EXT pins)
    uint8_t nmi_enable : 1;
} PPU_CONTROL;

typedef struct RP_2C02_PPU
{
    PPU_CONTROL PPUCTRL;    // $2000
    uint8_t PPUMASK;        // $2001
    PPU_STATUS PPUSTATUS;   // $2002
    uint8_t OAMADDR;        // $2003
    uint8_t OAMDATA;        // $2004
    uint16_t PPUSCROLL;     // $2005
    uint16_t PPUADDR;       // $2006
    uint8_t PPUDATA;        // $2007
    uint8_t OAMDMA;         // $4014

    bool w;                 // Write latch; Used by PPUSCROLL && PPUADDR
    bool odd_frame;

    uint8_t VRAM[0x1000];
    uint8_t palette_ram[32];
    uint8_t oam_memory[256];

    uint16_t scanline;
    uint16_t cycle;

    NES* nes;
} PPU;

void ppu_reset(PPU* ppu);
void ppu_power_up(PPU* ppu);
uint8_t ppu_read_byte(PPU* ppu, uint16_t address);
void ppu_write_byte(PPU* ppu, uint16_t address, uint8_t byte);
void ppu_cycle(PPU* ppu);