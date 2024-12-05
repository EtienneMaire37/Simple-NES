#pragma once

void ppu_reset(PPU* ppu)
{
    *(uint8_t*)&ppu->PPUCTRL = 0;
    ppu->PPUMASK = 0;
    ppu->PPUSCROLL = 0;
    ppu->PPUDATA = 0;

    ppu->w = 0;
    ppu->odd_frame = false;

    ppu->scanline = ppu->cycle = 0;
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
    {
        if (address >= 0x3000)
            return ppu_read_byte(ppu, address - 0x1000);

        if (ppu->mirroring == MR_VERTICAL)
        {
            if (address >= 0x2800)
                address -= 0x800;
        }
        else if (ppu->mirroring == MR_HORIZONTAL)
        {
            if (address >= 0x2400 && address < 0x2800)
                address -= 0x400;
            if (address >= 0x2c00 && address < 0x3000)
                address -= 0x400;
        }
        return ppu->VRAM[(address - 0x2000) % 0x1000];
    }

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
        if (address >= 0x3000)
        {
            ppu_write_byte(ppu, address - 0x1000, byte);
            return;
        }

        if (ppu->mirroring == MR_VERTICAL)
        {
            if (address >= 0x2800)
                address -= 0x800;
        }
        else if (ppu->mirroring == MR_HORIZONTAL)
        {
            if (address >= 0x2400 && address < 0x2800)
                address -= 0x400;
            if (address >= 0x2c00 && address < 0x3000)
                address -= 0x400;
        }
        ppu->VRAM[(address - 0x2000) % 0x1000] = byte;
        return;
    }

    ppu->palette_ram[(address - 0x3f00) % 32] = byte;
}

uint8_t ppu_read_palette(PPU* ppu, PALETTE_BG_SPRITE background_sprite, uint8_t palette_number, uint8_t index)
{
    return ppu_read_byte(ppu, (0x3f00 | (((uint16_t)background_sprite & 0b1) << 4) | ((palette_number & 0b11) << 2) | (index & 0b11))) & 0b111111;
}

void ppu_load_palette(PPU* ppu, char* path_to_palette)
{
    printf("Loading palette \"%s\"\n", path_to_palette);

    FILE* f = fopen(path_to_palette, "rb");

    if (f == NULL)
    {
        printf("Could'nt load palette.\n");
        return;
    }

    fread(&ppu->ntsc_palette[0], 192, 1, f);

    fclose(f);

    printf("Loading successful\n");
}

uint8_t ppu_read_pattern_table_plane_0(PPU* ppu, PATTERN_TABLE_SIDE side, uint8_t tile, uint8_t off_y)
{
    uint16_t address = (((uint16_t)side & 0b1) << 14) | ((uint16_t)tile << 4) | (off_y & 0b111);
    return ppu_read_byte(ppu, address);
}

uint8_t ppu_read_pattern_table_plane_1(PPU* ppu, PATTERN_TABLE_SIDE side, uint8_t tile, uint8_t off_y)
{
    uint16_t address = (((uint16_t)side & 0b1) << 14) | ((uint16_t)tile << 4) | 0b1000 | (off_y & 0b111);
    return ppu_read_byte(ppu, address);
}

uint8_t ppu_read_pattern_table(PPU* ppu, PATTERN_TABLE_SIDE side, uint8_t tile, uint8_t off_x, uint8_t off_y)
{
    off_x &= 0b111;
    return (ppu_read_pattern_table_plane_0(ppu, side, tile, off_y) >> (7 - off_x)) | ((ppu_read_pattern_table_plane_1(ppu, side, tile, off_y) >> (7 - off_x)) << 1);
}

void ppu_cycle(PPU* ppu)
{
    ppu->cycle++;
    if (ppu->cycle > 340)
    {
        ppu->cycle = 0;
        ppu->scanline++;
        if (ppu->scanline > 261)
            ppu->scanline = 0;
    }

    if (ppu->scanline == 241 && ppu->cycle == 1)
    {
        ppu->PPUSTATUS.vblank = true;
        ppu->nes->cpu.nmi = ppu->PPUCTRL.nmi_enable;
    }
    if (ppu->scanline == 261 && ppu->cycle == 1)
        ppu->PPUSTATUS.vblank = ppu->PPUSTATUS.sprite_0_hit = ppu->PPUSTATUS.sprite_overflow = false;

    if (ppu->cycle >= 1 && ppu->cycle <= 256 && ppu->scanline < 240)
    {
        uint8_t pix_x = (uint8_t)(ppu->cycle - 1);
        uint8_t pix_y = (uint8_t)ppu->scanline;

        uint8_t tile = (pix_x / 16) + ((pix_y / 16) * 16);
        uint8_t off_x = (pix_x % 16) / 2;
        uint8_t off_y = (pix_y % 16) / 2;

        uint8_t index = ppu_read_pattern_table(ppu, PT_LEFT, tile, off_x, off_y);

        uint8_t color_code = ppu_read_palette(ppu, PL_BACKGROUND, 1, index);

        ppu->screen[4 * ((uint16_t)pix_y * 256 + pix_x) + 0] = ppu->ntsc_palette[(color_code * 3 + 0) % 192];
        ppu->screen[4 * ((uint16_t)pix_y * 256 + pix_x) + 1] = ppu->ntsc_palette[(color_code * 3 + 1) % 192];
        ppu->screen[4 * ((uint16_t)pix_y * 256 + pix_x) + 2] = ppu->ntsc_palette[(color_code * 3 + 2) % 192];

        ppu->screen[4 * ((uint16_t)pix_y * 256 + pix_x) + 3] = 0xff;
    } 
}