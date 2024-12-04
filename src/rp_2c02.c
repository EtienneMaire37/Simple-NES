#pragma once

void ppu_reset(PPU* ppu)
{
    *(uint8_t*)&ppu->PPUCTRL = 0;
    ppu->PPUMASK = 0;
    ppu->PPUSCROLL = 0;
    ppu->PPUDATA = 0;

    ppu->word_latch = false;
    ppu->odd_frame = false;
}

void ppu_power_up(PPU* ppu)
{
    *(uint8_t*)&ppu->PPUSTATUS = 0b10100000;
    ppu->OAMADDR = 0;
    ppu->PPUADDR = 0;
    ppu_reset(ppu);
}

void ppu_cycle(PPU* ppu)
{
    ;
}