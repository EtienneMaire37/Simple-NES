#pragma once

void ppu_reset(PPU* ppu)
{
    *(uint8_t*)&ppu->PPUCTRL = 0;
    *(uint8_t*)&ppu->PPUMASK = 0;
    *(uint16_t*)&ppu->t = 0;
    ppu->x = 0;
    ppu->fine_x = 0;
    ppu->PPUDATA = 0;

    ppu->w = 0;
    ppu->odd_frame = false;

    ppu->scanline = 0;
    ppu->cycle = 1;
    ppu->frame_finished = false;
    ppu->can_nmi = true;

    ppu->horizontal_increment = ppu->vertical_increment = false;
}

void ppu_power_up(PPU* ppu)
{
    *(uint8_t*)&ppu->PPUSTATUS = 0b10100000;
    ppu->OAMADDR = 0;
    *(uint16_t*)&ppu->v = 0;
    ppu_reset(ppu);
    
    // From https://github.com/christopherpow/nes-test-roms/blob/master/blargg_ppu_tests_2005.09.15b/source/power_up_palette.asm
    uint8_t power_up_palette[32] = 
    {
        0x09, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x0D, 0x08, 0x10, 0x08, 0x24, 0x00, 0x00, 0x04, 0x2C,
        0x09, 0x01, 0x34, 0x03, 0x00, 0x04, 0x00, 0x14, 0x08, 0x3A, 0x00, 0x02, 0x00, 0x20, 0x2C, 0x08
    };
    memcpy(&ppu->palette_ram, &power_up_palette, 32);
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

        case MP_MMC1:
            if (!(ppu->nes->mmc1_control & 0b10000))
                return ppu->nes->CHR_ROM_data[(address + 0x1000 * (ppu->nes->selected_chrrom_bank_0 & 0b11110)) % ppu->nes->CHR_ROM_size];
            if (address < 0x1000)
                return ppu->nes->CHR_ROM_data[(address + 0x1000 * ppu->nes->selected_chrrom_bank_0) % ppu->nes->CHR_ROM_size];
            return ppu->nes->CHR_ROM_data[(address - 0x1000 + 0x1000 * ppu->nes->selected_chrrom_bank_1) % ppu->nes->CHR_ROM_size];

        case MP_UxROM:
            return ppu->nes->CHR_ROM_data[address % ppu->nes->CHR_ROM_size];

        case MP_AxROM:
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
            address = address & 0x7ff;
        }
        else if (ppu->mirroring == MR_HORIZONTAL)
        {
            address = ((address / 2) & 0x0400) + (address & 0x03ff);
        }
        else if (ppu->mirroring == MR_ONESCREEN_LOWER)
        {
            address = ((address - 0x2000) % 0x400);
        }
        else if (ppu->mirroring == MR_ONESCREEN_HIGHER)
        {
            address = 0xc00 + ((address - 0x2000) % 0x400);
        }
        return ppu->VRAM[address % 0x1000];
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
        if (ppu->nes->CHR_RAM)
        {
            switch (ppu->nes->mapper)
            {
            case MP_NROM:
                ppu->nes->CHR_ROM_data[address % ppu->nes->CHR_ROM_size] = byte;
                return;

            case MP_MMC1:
                if (!(ppu->nes->mmc1_control & 0b10000))
                {
                    ppu->nes->CHR_ROM_data[(address + 0x1000 * (ppu->nes->selected_chrrom_bank_0 & 0b11110)) % ppu->nes->CHR_ROM_size] = byte;
                    return;
                }
                if (address < 0x1000)
                {
                    ppu->nes->CHR_ROM_data[(address + 0x1000 * ppu->nes->selected_chrrom_bank_0) % ppu->nes->CHR_ROM_size] = byte;
                    return;
                }
                ppu->nes->CHR_ROM_data[(address - 0x1000 + 0x1000 * ppu->nes->selected_chrrom_bank_1) % ppu->nes->CHR_ROM_size] = byte;
                return;

            case MP_UxROM:
                ppu->nes->CHR_ROM_data[address % ppu->nes->CHR_ROM_size] = byte;
                return;

            case MP_AxROM:
                ppu->nes->CHR_ROM_data[address % ppu->nes->CHR_ROM_size] = byte;
                return;
            }
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
            address = address & 0x7ff;
        }
        else if (ppu->mirroring == MR_HORIZONTAL)
        {
            address = ((address / 2) & 0x0400) + (address & 0x03ff);
        }
        else if (ppu->mirroring == MR_ONESCREEN_LOWER)
        {
            address = ((address - 0x2000) % 0x400);
        }
        else if (ppu->mirroring == MR_ONESCREEN_HIGHER)
        {
            address = 0xc00 + ((address - 0x2000) % 0x400);
        }
        ppu->VRAM[address % 0x1000] = byte;
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
    off_x &= 0b111;
    return ((ppu_read_pattern_table_plane_0(ppu, side, tile, off_y) >> (7 - off_x)) & 0b1) | (((ppu_read_pattern_table_plane_1(ppu, side, tile, off_y) >> (7 - off_x)) << 1) & 0b10);
}

uint8_t ppu_read_nametable(PPU* ppu, uint8_t nametable, uint16_t bg_tile)
{
    return ppu_read_byte(ppu, 0x2000 + (nametable & 0b11) * 0x400 + bg_tile);
}

void ppu_cycle(PPU* ppu)
{    
    ppu->cycle++;
    if (ppu->cycle > 340)
    {
        ppu->cycle = 0;
        ppu->scanline++;
        if (ppu->scanline > ppu_prerender_scanline(ppu))
        {
            ppu->scanline = 0;
            memcpy(ppu->screen_buffer, ppu->screen, 256 * 240 * 4);
            ppu->frame_finished = true;
        }
    }

    if (ppu->scanline < 240)
    {
        if (ppu->cycle >= 1 && ppu->cycle <= 256)
        {
            uint8_t color_code = 0, bg_color_code = 0, sprite_color_code = 0;
            uint8_t image_pix_x = (uint8_t)(ppu->cycle - 1);
            uint8_t image_pix_y = (uint8_t)ppu->scanline;

            struct OAM_SPRITE_ENTRY rendered_sprite;
            rendered_sprite.attributes.priority = 1;

            bool sprite_transparent_pixel = true, bg_transparent_pixel = true;

            if (ppu->PPUMASK.enable_bg && (ppu->PPUMASK.show_bg_left || image_pix_x >= 8))
            {
                uint8_t pix_x = ppu->v.coarse_x * 8 + ppu->fine_x;
                uint8_t pix_y = ppu->v.coarse_y * 8 + ppu->v.fine_y;

                uint16_t bg_tile = ppu->v.coarse_x + 32 * ppu->v.coarse_y;
                uint16_t palette_tile = (pix_x / 32) + ((pix_y / 32) * 8);
                uint8_t palette_off_x = pix_x % 32;
                uint8_t palette_off_y = pix_y % 32;

                uint8_t pattern_tile = ppu_read_nametable(ppu, ppu->v.nametable_select, bg_tile);
                uint8_t off_x = ppu->fine_x; 
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

                if (index != 0)
                {
                    bg_color_code = ppu_read_palette(ppu, PL_BACKGROUND, palette, index);
                    bg_transparent_pixel = false;
                }
            } 

            if (ppu->PPUMASK.enable_sprites && (ppu->PPUMASK.show_sprites_left || image_pix_x >= 8))
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
                        {
                            if (ppu->PPUCTRL.sprite_size)   // 8x16 sprite
                                off_y = 15 - off_y;
                            else
                                off_y = 7 - off_y;
                        }
                        if (ppu->PPUCTRL.sprite_size)
                        {
                            if (off_y >= 8)
                                palette_index = ppu_read_pattern_table(ppu, (sprite.tile_index & 1), (sprite.tile_index & 0b11111110) | 1, off_x, off_y - 8);
                            else
                                palette_index = ppu_read_pattern_table(ppu, (sprite.tile_index & 1), (sprite.tile_index & 0b11111110), off_x, off_y);
                        }
                        else
                            palette_index = ppu_read_pattern_table(ppu, ppu->PPUCTRL.sprite_pattern_table_address, sprite.tile_index, off_x, off_y);
                        
                        if (palette_index != 0)
                        {
                            sprite_transparent_pixel = false;
                            sprite_color_code = ppu_read_palette(ppu, PL_SPRITE, sprite.attributes.palette, palette_index);
                            rendered_sprite = sprite;

                            if (index == 0 && ppu->sprite_0_rendered && !bg_transparent_pixel && image_pix_x != 255) // Sprite 0 hit
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
            {
                if (*(uint16_t*)&ppu->v >= 0x3f00 && ppu_forced_blanking(ppu))  // Backdrop override
                    color_code = ppu_read_byte(ppu, *(uint16_t*)&ppu->v);
                else
                    color_code = ppu_read_palette(ppu, PL_SPRITE, 0, 0);
            }

            if (ppu->PPUMASK.grayscale)
                color_code &= 0x30;
            uint8_t r = ppu->ntsc_palette[(color_code * 3 + 0) % 192], g = ppu->ntsc_palette[(color_code * 3 + 1) % 192], b = ppu->ntsc_palette[(color_code * 3 + 2) % 192];
            if ((ppu->PPUMASK.emphasize_red || ppu->PPUMASK.emphasize_green || ppu->PPUMASK.emphasize_blue) && (color_code & 0x0f) != 0x0f)
            {
                if (ppu->PPUMASK.emphasize_red && ppu->PPUMASK.emphasize_green && ppu->PPUMASK.emphasize_blue)
                {
                    r *= 0.816328;
                    g *= 0.816328;
                    b *= 0.816328;
                }
                else
                {
                    if (!ppu->PPUMASK.emphasize_red)
                        r *= 0.816328;
                    if (!ppu->PPUMASK.emphasize_green)
                        g *= 0.816328;
                    if (!ppu->PPUMASK.emphasize_blue)
                        b *= 0.816328;
                }
            }

            ppu->screen[4 * ((uint16_t)image_pix_y * 256 + image_pix_x) + 0] = r;
            ppu->screen[4 * ((uint16_t)image_pix_y * 256 + image_pix_x) + 1] = g;
            ppu->screen[4 * ((uint16_t)image_pix_y * 256 + image_pix_x) + 2] = b;
            
            ppu->screen[4 * ((uint16_t)image_pix_y * 256 + image_pix_x) + 3] = 0xff;

            if (ppu_rendering_enabled(ppu))
            {
                ppu->horizontal_increment = true;

                if (ppu->cycle == 256)
                    ppu->vertical_increment = true;
            }
        }
    }

    if (ppu->horizontal_increment)
    {
        ppu->fine_x++;
        if (ppu->fine_x > 0b111)
        {
            ppu->fine_x = 0;
            if (ppu->v.coarse_x == 0b11111)
            {
                ppu->v.coarse_x = 0;
                ppu->v.nametable_select ^= 0b01;    // Switch horizontal nametable
            }
            else
                ppu->v.coarse_x++;
        }
    }

    if (ppu->vertical_increment)
    {
        if (ppu->v.fine_y < 0b111)        
            ppu->v.fine_y++;
        else
        {
            ppu->v.fine_y = 0;
            uint8_t y = ppu->v.coarse_y;
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

    ppu->horizontal_increment = ppu->vertical_increment = false;

    if (ppu_rendering_enabled(ppu))
    {
        if (ppu->scanline < 240 || ppu->scanline == ppu_prerender_scanline(ppu))
        {
            if (ppu->cycle >= 1 && ppu->cycle <= 64)
            {
                if (ppu->cycle % 2 == 0)
                    ppu->secondary_oam_memory[(ppu->cycle - 1) / 2] = 0xff; // ppu_read_byte(ppu, 0x2004);
            }
        }
        if (ppu->scanline < 240)
        {
            if (ppu->cycle >= 65 && ppu->cycle <= 256)
            {
                uint8_t n = (ppu->cycle - 65) / 2;
                if (ppu->cycle <= 192)
                {
                    if (ppu->cycle == 65)
                        ppu->sprite_index = ppu->sprite_0_prepared = ppu->sprite_eval_m = 0;
                    if (ppu->cycle % 2 == 1)    // Odd cycle
                    {
                        ppu->oam_byte_read = ppu->oam_memory[4 * n];
                    }
                    else        // Even cycle
                    {
                        if (ppu->sprite_index < 8)  // != 8
                        {
                            ppu->secondary_oam_memory[4 * ppu->sprite_index] = ppu->oam_byte_read;
                            int16_t off_y = ppu->scanline - ppu->secondary_oam_memory[4 * ppu->sprite_index];
                            if (off_y >= 0 && off_y < (ppu->PPUCTRL.sprite_size ? 16 : 8))
                            {
                                ppu->secondary_oam_memory[4 * ppu->sprite_index + 1] = ppu->oam_memory[4 * n + 1];
                                ppu->secondary_oam_memory[4 * ppu->sprite_index + 2] = ppu->oam_memory[4 * n + 2];
                                ppu->secondary_oam_memory[4 * ppu->sprite_index + 3] = ppu->oam_memory[4 * n + 3];
                                
                                ppu->sprite_index++;

                                if (n == 0)
                                    ppu->sprite_0_prepared = true;
                            }
                            else
                            {
                                ppu->sprite_eval_m = 0;
                                do
                                {
                                    int16_t off_y = ppu->scanline - ppu->secondary_oam_memory[4 * ppu->sprite_index + ppu->sprite_eval_m];
                                    if (off_y >= 0 && off_y < (ppu->PPUCTRL.sprite_size ? 16 : 8))
                                    {
                                        ppu->PPUSTATUS.sprite_overflow = true;
                                        ppu->sprite_eval_m = 0; // += 3;
                                    }
                                    else
                                    {
                                        ppu->sprite_eval_m++;
                                        ppu->sprite_eval_m &= 0b11;
                                    }
                                } while (ppu->sprite_eval_m != 0);
                            }
                        }
                    }
                    // if (ppu->cycle != 65 && ppu->cycle % 2 == 1)
                    //     ppu->OAMADDR += 4;
                }
                // else if (ppu->cycle <= 256)
                //     ppu->OAMADDR += 4;
            }
        }
    }

    if (ppu_rendering_enabled(ppu))
    {       
        if (ppu->scanline == ppu_prerender_scanline(ppu))
        {
            if (ppu->cycle == 280)  // 280 to 304 but i'm doing it in one go there
            {
                ppu->v.coarse_y = ppu->t.coarse_y;
                ppu->v.fine_y = ppu->t.fine_y;
                ppu->v.nametable_select = (ppu->t.nametable_select & 0b10) | (ppu->v.nametable_select & 0b01);
            }
        }
    
        if (ppu->scanline < 239 || ppu->scanline == ppu_prerender_scanline(ppu))
        {
            if (ppu->cycle == 257)
            {
                ppu->fine_x = ppu->x;
                ppu->v.coarse_x = ppu->t.coarse_x;
                ppu->v.nametable_select = (ppu->t.nametable_select & 0b01) | (ppu->v.nametable_select & 0b10);
            }
        }
    }

    if (ppu->cycle == 257)
    {
        memcpy(&ppu->sprites_to_render[0], &ppu->secondary_oam_memory, 32);
        ppu->num_sprites_to_render = ppu->sprite_index;
        ppu->sprite_0_rendered = ppu->sprite_0_prepared;
    }

    if (ppu->scanline == ppu_prerender_scanline(ppu) && ppu->cycle == 65)
    {
        ppu->num_sprites_to_render = 0;
        ppu->sprite_0_rendered = false;
    }

    if (ppu->scanline == ppu_prerender_scanline(ppu) && ppu->cycle == 1)
        ppu->PPUSTATUS.vblank = ppu->PPUSTATUS.sprite_0_hit = ppu->PPUSTATUS.sprite_overflow = 
        ppu->nes->cpu.nmi = false;

    if (ppu->nes->cpu.nmi_latch < 5)    ppu->nes->cpu.nmi_latch++;
        
    if (ppu->scanline == 241 && ppu->cycle == 1)
    {
        if (ppu->can_nmi)
        {
            ppu->PPUSTATUS.vblank = true;
            if (ppu->PPUCTRL.nmi_enable) ppu->nes->cpu.nmi = true;
            ppu->nes->cpu.nmi_latch = 0;
        }
    }
    ppu->can_nmi = true;

    if (ppu->nes->system == TV_NTSC)
    {
        if (ppu->cycle == 338 && ppu->scanline == 261)
        {
            ppu->odd_frame ^= true;
            if (ppu->odd_frame && ppu->PPUMASK.enable_bg)
                ppu->cycle++;
        }
    }
}