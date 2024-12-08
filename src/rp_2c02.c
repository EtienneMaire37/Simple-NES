#pragma once

void ppu_reset(PPU* ppu)
{
    *(uint8_t*)&ppu->PPUCTRL = 0;
    *(uint8_t*)&ppu->PPUMASK = 0;
    *(uint16_t*)&ppu->t = 0;
    ppu->x = 0;
    ppu->PPUDATA = 0;

    ppu->w = 0;
    ppu->odd_frame = false;

    ppu->scanline = 0;
    ppu->cycle = 0;
}

void ppu_power_up(PPU* ppu)
{
    *(uint8_t*)&ppu->PPUSTATUS = 0b10100000;
    ppu->OAMADDR = 0;
    *(uint16_t*)&ppu->v = 0;
    ppu_reset(ppu);
}

uint8_t ppu_read_byte(PPU* ppu, uint16_t address)
{
    address &= 0x3fff;

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

    if (address == 0x3f10)
        return ppu->palette_ram[0];
    return ppu->palette_ram[(address - 0x3f00) % 32];
}

void ppu_write_byte(PPU* ppu, uint16_t address, uint8_t byte)
{
    address &= 0x3fff;

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

    if (address == 0x3f10)
        ppu->palette_ram[0] = byte;
    else
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
    uint16_t address = (((uint16_t)side & 0b1) * 0x1000) | ((uint16_t)tile << 4) | (off_y & 0b111);
    return ppu_read_byte(ppu, address);
}

uint8_t ppu_read_pattern_table_plane_1(PPU* ppu, PATTERN_TABLE_SIDE side, uint8_t tile, uint8_t off_y)
{
    uint16_t address = (((uint16_t)side & 0b1) * 0x1000) | ((uint16_t)tile << 4) | 0b1000 | (off_y & 0b111);
    return ppu_read_byte(ppu, address);
}

uint8_t ppu_read_pattern_table(PPU* ppu, PATTERN_TABLE_SIDE side, uint8_t tile, uint8_t off_x, uint8_t off_y)
{
    off_x &= 0b111;
    return ((ppu_read_pattern_table_plane_0(ppu, side, tile, off_y) >> (7 - off_x)) & 0b1) | (((ppu_read_pattern_table_plane_1(ppu, side, tile, off_y) >> (7 - off_x)) << 1) & 0b10);
}

uint8_t ppu_read_nametable(PPU* ppu, uint8_t nametable, uint16_t bg_tile)
{
    return ppu_read_byte(ppu, 0x2000 + (nametable & 0b11) * 0x400 + bg_tile);
}

void ppu_cycle(PPU* ppu)
{
    if (ppu->PPUMASK.enable_bg || ppu->PPUMASK.enable_sprites)
    {       
        if (ppu->scanline == 261)
        {
            if (ppu->cycle == 280)  // 280 to 304 but i'm doing it in one go there
            {
                ppu->v.coarse_y = ppu->t.coarse_y;
                ppu->v.fine_y = ppu->t.fine_y;
                ppu->v.nametable_select = (ppu->t.nametable_select & 0b10) | (ppu->v.nametable_select & 0b01);
            }
        }
    
        if (ppu->scanline <= 240 || ppu->scanline == 261)
        {
            if (ppu->cycle == 257)
            {
                ppu->v.coarse_x = ppu->t.coarse_x;
                ppu->v.nametable_select = (ppu->t.nametable_select & 0b01) | (ppu->v.nametable_select & 0b10);
            }
        }
    }

    if (ppu->scanline < 240)
    {
        if (ppu->PPUMASK.enable_sprites && ppu->cycle == 256)
        {
            ppu->num_sprites_to_render = 0;
            struct OAM_SPRITE_ENTRY sprite;
            uint8_t index;
            int16_t off_y;
            for (uint8_t i = 0; i < 64 && ppu->num_sprites_to_render < 8; i++)
            {
                sprite = *(struct OAM_SPRITE_ENTRY*)&ppu->oam_memory[i * 4];
                if (sprite.sprite_y < 240)
                {
                    off_y = ppu->scanline - sprite.sprite_y;
                    if (off_y >= 0 && off_y < 8)
                    {
                        ppu->sprites_to_render[ppu->num_sprites_to_render] = sprite;
                        ppu->num_sprites_to_render++;
                    }
                }
            }
        }

        if (ppu->cycle >= 1 && ppu->cycle <= 256)
        {
            uint8_t color_code = 0, bg_color_code = 0, sprite_color_code = 0;
            uint8_t image_pix_x = (uint8_t)(ppu->cycle - 1);
            uint8_t image_pix_y = (uint8_t)ppu->scanline;

            struct OAM_SPRITE_ENTRY rendered_sprite;
            rendered_sprite.attributes.priority = 1;

            bool sprite_transparent_pixel = true, bg_transparent_pixel = !ppu->PPUMASK.enable_bg;

            if (ppu->PPUMASK.enable_bg)
            {
                uint8_t pix_x = ppu->v.coarse_x * 8 + ppu->x;
                uint8_t pix_y = ppu->v.coarse_y * 8 + ppu->v.fine_y;

                uint16_t bg_tile = ppu->v.coarse_x + 32 * ppu->v.coarse_y;
                uint16_t palette_tile = (pix_x / 32) + ((pix_y / 32) * 8);
                uint8_t palette_off_x = pix_x % 32;
                uint8_t palette_off_y = pix_y % 32;

                uint8_t pattern_tile = ppu_read_nametable(ppu, ppu->v.nametable_select, bg_tile);
                uint8_t off_x = ppu->x; 
                uint8_t off_y = ppu->v.fine_y;

                uint8_t palette_byte = ppu_read_nametable(ppu, ppu->v.nametable_select, 960 + palette_tile);
                uint8_t palette = 0;

                if (palette_off_x < 16 && palette_off_y < 16)
                    palette = palette_byte & 0b11;
                if (palette_off_x >= 16 && palette_off_y < 16)
                    palette = (palette_byte >> 2) & 0b11;
                if (palette_off_x < 16 && palette_off_y >= 16)
                    palette = (palette_byte >> 4) & 0b11;
                if (palette_off_x >= 16 && palette_off_y >= 16)
                    palette = (palette_byte >> 6) & 0b11;

                uint8_t index = ppu_read_pattern_table(ppu, ppu->PPUCTRL.background_pattern_table_address, pattern_tile, off_x, off_y);

                if (index == 0)
                    bg_transparent_pixel = true;
                else
                    bg_color_code = ppu_read_palette(ppu, PL_BACKGROUND, palette, index);
            } 

            if (ppu->PPUMASK.enable_sprites)
            {
                uint8_t index;
                int16_t off_x, off_y;
                uint8_t palette_index;
                struct OAM_SPRITE_ENTRY sprite;
                for (uint8_t i = 0; i < ppu->num_sprites_to_render; i++)
                {
                    index = ppu->num_sprites_to_render - i - 1;
                    sprite = ppu->sprites_to_render[index];
                    
                    off_x = image_pix_x - sprite.sprite_x;
                    off_y = image_pix_y - sprite.sprite_y - 1;
                    if (off_x >= 0 && off_x < 8)
                    {
                        if (sprite.attributes.flip_x)
                            off_x = 7 - off_x;
                        if (sprite.attributes.flip_y)
                            off_y = 7 - off_y;
                        palette_index = ppu_read_pattern_table(ppu, ppu->PPUCTRL.sprite_pattern_table_address, sprite.tile_index, off_x, off_y);
                        if (palette_index != 0)
                        {
                            sprite_transparent_pixel = false;
                            sprite_color_code = ppu_read_palette(ppu, PL_SPRITE, sprite.attributes.palette, palette_index);
                            rendered_sprite = sprite;

                            if (index == 0 && !bg_transparent_pixel) // Sprite 0 hit
                                ppu->PPUSTATUS.sprite_0_hit = true;
                        }
                    }
                }
            }

            if ((!sprite_transparent_pixel) || (!bg_transparent_pixel))
            {
                if (!bg_transparent_pixel)
                {
                    if ((!sprite_transparent_pixel) && (!rendered_sprite.attributes.priority))
                    {
                        color_code = sprite_color_code;
                    }
                    else
                    {
                        color_code = bg_color_code;
                    }
                }
                else
                    color_code = sprite_color_code;
            }
            else
                color_code = ppu_read_palette(ppu, PL_SPRITE, 0, 0);

            if (ppu->PPUMASK.grayscale)
                color_code &= 0x30;
            uint8_t r = ppu->ntsc_palette[(color_code * 3 + 0) % 192], g = ppu->ntsc_palette[(color_code * 3 + 1) % 192], b = ppu->ntsc_palette[(color_code * 3 + 2) % 192];
            if ((ppu->PPUMASK.emphasize_red || ppu->PPUMASK.emphasize_green || ppu->PPUMASK.emphasize_blue) && color_code != 0x0f)
            {
                if (!ppu->PPUMASK.emphasize_red)
                    r *= 0.816328;
                if (!ppu->PPUMASK.emphasize_green)
                    g *= 0.816328;
                if (!ppu->PPUMASK.emphasize_blue)
                    b *= 0.816328;
            }
            ppu->screen[4 * ((uint16_t)image_pix_y * 256 + image_pix_x) + 0] = r;
            ppu->screen[4 * ((uint16_t)image_pix_y * 256 + image_pix_x) + 1] = g;
            ppu->screen[4 * ((uint16_t)image_pix_y * 256 + image_pix_x) + 2] = b;
            
            ppu->screen[4 * ((uint16_t)image_pix_y * 256 + image_pix_x) + 3] = 0xff;

            if (ppu->PPUMASK.enable_bg || ppu->PPUMASK.enable_sprites)
            {
                ppu->x++;
                if (ppu->x > 0b111)
                {
                    ppu->x = 0;
                    if (ppu->v.coarse_x == 0b11111)
                    {
                        ppu->v.coarse_x = 0;
                        ppu->v.nametable_select ^= 0b01;    // Switch horizontal nametable
                    }
                    else
                        ppu->v.coarse_x++;
                }

                if (ppu->cycle == 256)
                {
                    if (ppu->v.fine_y < 0b111)        
                        ppu->v.fine_y++;
                    else
                    {
                        ppu->v.fine_y = 0;
                        int y = ppu->v.coarse_y;
                        if (y == 29)
                        {
                            y = 0;
                            ppu->v.nametable_select ^= 0b10;     // Switch vertical nametable
                        }
                        else if (y == 0b11111)
                            y = 0;
                        else
                            y++;
                        ppu->v.coarse_y = y;
                    }
                }
            }
        }
    }
    
    if (ppu->scanline == 241 && ppu->cycle == 1)
    {
        ppu->PPUSTATUS.vblank = true;
        if (ppu->PPUCTRL.nmi_enable) ppu->nes->cpu.nmi = true;
    }

    if (ppu->scanline == 261 && ppu->cycle == 1)
        ppu->PPUSTATUS.vblank = ppu->PPUSTATUS.sprite_0_hit = ppu->PPUSTATUS.sprite_overflow = false;
    
    ppu->cycle++;
    if (ppu->cycle > 340)
    {
        ppu->cycle = 0;
        ppu->scanline++;
        if (ppu->scanline > 261)
            ppu->scanline = 0;
    }
}