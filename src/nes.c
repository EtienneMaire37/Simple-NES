#pragma once

NES nes_create()
{
    NES nes;
    nes.mapper = MP_UNSUPPORTED;
    nes.master_clock = 0;

    nes.PRG_ROM_data = NULL;
    nes.PRG_ROM_size = 0;

    nes.CHR_ROM_data = NULL;
    nes.CHR_ROM_size = 0;

    nes.PRG_RAM_data = NULL;

    nes.created = NES_CREATED_MAGIC_DWORD;

    return nes;
}

void nes_init(NES* nes)
{
    nes->cpu.nes = nes;
    nes->ppu.nes = nes;
    nes->apu.nes = nes;

    apu_init(&nes->apu);
}

void nes_init_mmc1(NES* nes)
{
    nes->mmc1_bits_shifted = nes->mmc1_shift_register = 0;
    nes->mmc1_control = 0b11100;    // Fix last bank to 0xc000-0xffff
}

void nes_destroy(NES* nes)
{
    free(nes->PRG_ROM_data);
    nes->PRG_ROM_data = NULL;
    free(nes->CHR_ROM_data);
    nes->CHR_ROM_data = NULL;
    free(nes->PRG_RAM_data);
    nes->PRG_RAM_data = NULL;

    apu_destroy(&nes->apu);

    nes->PRG_ROM_size = 0;
    nes->CHR_ROM_size = 0;
}

void nes_reset(NES* nes)
{
    if (nes->created != NES_CREATED_MAGIC_DWORD)
    {
        printf("NES object used while not initialized\n");
        return;
    }

    nes_init_mmc1(nes);
    
    cpu_reset(&nes->cpu);
    apu_reset(&nes->apu);
    ppu_reset(&nes->ppu);

    nes->key_strobe = false;
    nes->key_status_control = nes->key_status = 0;
}

void nes_power_up(NES* nes)
{
    if (nes->created != NES_CREATED_MAGIC_DWORD)
    {
        printf("NES object used while not initialized\n");
        return;
    }

    nes_init_mmc1(nes);

    cpu_power_up(&nes->cpu);
    apu_reset(&nes->apu);
    ppu_power_up(&nes->ppu);

    nes->master_clock = rand() % (nes->system == TV_NTSC ? 4 : 5);
}

void nes_handle_controls(NES* nes)
{
    for (uint8_t j = 0; j < 8; j++)
    {
        nes->key_status_control <<= 1;
        nes->key_status_control |= sfKeyboard_isKeyPressed(keymap[j]);
    }

    if (((nes->key_status_control & 0b00001000) != 0) && ((nes->key_status_control & 0b00000100) != 0)) // up and down
        nes->key_status_control &= 0b11110011;
    if (((nes->key_status_control & 0b00000010) != 0) && ((nes->key_status_control & 0b00000001) != 0)) // left and right
        nes->key_status_control &= 0b11111100;
}

void nes_cycle(NES* nes)
{
    if (nes->created != NES_CREATED_MAGIC_DWORD)
    {
        printf("NES object used while not initialized\n");
        return;
    }

    if (nes->key_strobe)
        nes->key_status = nes->key_status_control;

    nes->master_clock++;

    if (nes->master_clock % (nes->system == TV_NTSC ? 4 : 5) == 0)
        ppu_cycle(&nes->ppu);
    
    if (nes->master_clock % (nes->system == TV_NTSC ? 12 : 16) == 0)
    {
        cpu_cycle(&nes->cpu);
        nes->apu.cpu_cycles++;
        nes->apu.cpu_cycles %= (nes->system == TV_NTSC ? (nes->apu.sequencer_mode ? 18641 : 14915) : (nes->apu.sequencer_mode ? 20783 : 16627)) * 2;   // those are in apu cycles
        apu_cycle(&nes->apu);
    }
}

void nes_load_game(NES* nes, char* path_to_rom)
{
    if (nes->created != NES_CREATED_MAGIC_DWORD)
    {
        printf("NES object used while not initialized\n");
        return;
    }
    
    printf("Loading rom \"%s\"\n", path_to_rom);

    struct iNES_HEADER header;
    uint16_t mapper_number = 0xffff;

    FILE* f = fopen(path_to_rom, "rb");

    if (f == NULL)
    {
        printf("Couldn't load rom.\n");
        return;
    }

    fread(&header, sizeof(header), 1, f);

    if (header.flags_6.trainer)
    {
        fseek(f, 512, SEEK_CUR);     // Skip the trainer
        printf("    Skipping trainer\n");
    }

    if (nes->PRG_ROM_data != NULL)
    {
        printf("    PRG ROM data already allocated\n");
        free(nes->PRG_ROM_data);
    }

    nes->PRG_ROM_size = 16384 * header.prg_rom;
    printf("    Allocating %u bytes of PRG ROM data\n", nes->PRG_ROM_size);
    nes->PRG_ROM_data = (uint8_t*)malloc(nes->PRG_ROM_size);

    if (nes->PRG_ROM_data == NULL)
    {
        printf("    Couldn't allocate data for the PRG ROM\n");
        return;
    }

    fread(nes->PRG_ROM_data, nes->PRG_ROM_size, 1, f);

    if (nes->CHR_ROM_data != NULL)
    {
        printf("    CHR ROM data already allocated\n");
        free(nes->CHR_ROM_data);
    }

    nes->CHR_ROM_size = 8192 * header.chr_rom;
    nes->CHR_RAM = false;
    if (nes->CHR_ROM_size == 0)     // CHR RAM
    {
        nes->CHR_ROM_size = 8192;
        nes->CHR_RAM = true;
    }
    printf("    Allocating %u bytes of CHR R%cM data\n", nes->CHR_ROM_size, nes->CHR_RAM ? 'A' : 'O');
    nes->CHR_ROM_data = (uint8_t*)malloc(nes->CHR_ROM_size);

    if (nes->CHR_ROM_data == NULL)
    {
        printf("    Couldn't allocate data for the CHR ROM\n");
        return;
    }

    if (!nes->CHR_RAM)
        fread(nes->CHR_ROM_data, nes->CHR_ROM_size, 1, f);

    fclose(f);

    bool nes_20 = header.flags_7.NES_20 == 2;   // https://www.nesdev.org/wiki/INES#Flags_7 says 2 but then says 1 later in the article

    if (nes_20)
    {
        printf("    NES 2.0 file format\n");
        // return;
    }

    mapper_number = header.flags_6.mapper_lo | (header.flags_7.mapper_hi << 4);

    printf("    Mapper: %u\n", mapper_number);

    nes->system = (header.flags_9.tv_system & 1) | (header.flags_10.tv_system == 2) | (header.flags_12 & 1);
    printf("    TV system: %s\n", system_text[nes->system]);

    if (mapper_number == 71)    mapper_number = 2;

    if (!(mapper_number == 0 || mapper_number == 1 || mapper_number == 2 || mapper_number == 7))
    {
        nes->mapper = MP_UNSUPPORTED;
        printf("Mapper unsupported\n", mapper_number);
        return;
    }
    else
        nes->mapper = (MAPPER)mapper_number;

    NAMETABLE_MIRRORING mirroring = MR_ALTERNATIVE;

    if (!header.flags_6.other_nametable_layout)
        mirroring = header.flags_6.nametable_layout;

    nes->ppu.mirroring = mirroring;

    nes->PRG_RAM_size = (uint32_t)header.flags_8.prg_ram_size * 8192;
    if (nes->PRG_RAM_size == 0) nes->PRG_RAM_size = 8192;

    printf("    Allocating %u bytes of PRG RAM\n", nes->PRG_RAM_size);
    if (nes->PRG_RAM_data == NULL)
        nes->PRG_RAM_data = (uint8_t*)malloc(nes->PRG_RAM_size);
    else
    {
        printf("    PRG RAM already allocated\n");
        free(nes->PRG_RAM_data);
        nes->PRG_RAM_data = (uint8_t*)malloc(nes->PRG_RAM_size);
    }

    nes->selected_prgrom_bank_0 = nes->selected_prgram_bank = nes->selected_chrrom_bank_0 = nes->selected_chrrom_bank_1 = 0;

    if (mapper_number == 1)
        nes_init_mmc1(nes);

    if (mapper_number == 7)
        nes->ppu.mirroring = MR_ONESCREEN_LOWER;

    printf("    Mirroring: %s\n", mirroring_text[(uint8_t)mirroring]);
    printf("    Entry point: 0x%x\n", cpu_read_word(&nes->cpu, CPU_RESET_VECTOR));

    printf("Loading successful\n");
}