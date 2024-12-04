#pragma once

void ppu_reset(PPU* ppu)
{
    *(uint8_t*)&ppu->PPUCTRL = 0;
    ppu->PPUMASK = 0;
    ppu->PPUSCROLL = 0;
    ppu->PPUDATA = 0;

    ppu->w = 0;
    ppu->odd_frame = false;
}

void ppu_power_up(PPU* ppu)
{
    *(uint8_t*)&ppu->PPUSTATUS = 0b10100000;
    ppu->OAMADDR = 0;
    ppu->PPUADDR = 0;
    ppu_reset(ppu);
}

uint8_t ppu_read_byte(PPU* ppu, uint16_t address)
{
    if (address < 0x2000)
    {
        switch (ppu->nes->mapper)
        {
        case MP_NROM:
            return ppu->nes->CHR_ROM_data[address % ppu->nes->CHR_ROM_size];
        }
        return 0;
    }

    if (address < 0x3f00)
        return ppu->VRAM[(address - 0x2000) % 0x1000];

    return ppu->palette_ram[(address - 0x3f00) % 32];
}

void ppu_write_byte(PPU* ppu, uint16_t address, uint8_t byte)
{
    if (address < 0x2000)
    {
        switch (ppu->nes->mapper)
        {
        case MP_NROM:   // CHR ROM
            return;
        }
        return;
    }

    if (address < 0x3f00)
    {
        ppu->VRAM[(address - 0x2000) % 0x1000] = byte;
        return;
    }

    ppu->palette_ram[(address - 0x3f00) % 32] = byte;
}

void ppu_cycle(PPU* ppu)
{
    ;
}