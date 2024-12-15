#pragma once

NES nes_create()
{
    NES nes;
    nes.mapper = MP_UNSUPPORTED;
    nes.cycle_alignment = 0;

    nes.PRG_ROM_data = NULL;
    nes.PRG_ROM_size = 0;

    nes.CHR_ROM_data = NULL;
    nes.CHR_ROM_size = 0;

    nes.PRG_RAM = NULL;

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

    nes->selected_prgrom_bank_0 = nes->selected_prgram_bank = nes->selected_chrrom_bank_0 = nes->selected_chrrom_bank_1 = 0;
}

void nes_destroy(NES* nes)
{
    free(nes->PRG_ROM_data);
    nes->PRG_ROM_data = NULL;
    free(nes->CHR_ROM_data);
    nes->CHR_ROM_data = NULL;
    free(nes->PRG_RAM);
    nes->PRG_RAM = NULL;

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

    nes->key_strobe = true;
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

    nes->cycle_alignment = rand() % 3;
}

void nes_cycle(NES* nes)
{
    if (nes->created != NES_CREATED_MAGIC_DWORD)
    {
        printf("NES object used while not initialized\n");
        return;
    }
    
    nes->cycle_alignment++;
    nes->cycle_alignment %= 6;

    if (nes->key_strobe)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            nes->key_status <<= 1;
            nes->key_status |= sfKeyboard_isKeyPressed(keymap[i]);
        }

        if (((nes->key_status & 0b00001000) != 0) && ((nes->key_status & 0b00000100) != 0)) // up and down
            nes->key_status &= 0b11110011;
        if (((nes->key_status & 0b00000010) != 0) && ((nes->key_status & 0b00000001) != 0)) // left and right
            nes->key_status &= 0b11111100;
    }

    if (nes->cycle_alignment % 3 == 0)
    {
        cpu_cycle(&nes->cpu);
        nes->apu.cpu_cycles++;
        nes->apu.cpu_cycles %= (nes->apu.sequencer_mode ? 18641 : 14915) * 2;   // those are in apu cycles
        apu_cycle(&nes->apu);
    }
        
    ppu_cycle(&nes->ppu);
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
        printf("Could'nt load rom.\n");
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
        printf("    Could'nt allocate data for the PRG ROM\n");
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
        printf("    Could'nt allocate data for the CHR ROM\n");
        return;
    }

    if (!nes->CHR_RAM)
        fread(nes->CHR_ROM_data, nes->CHR_ROM_size, 1, f);

    fclose(f);

    if (header.flags_7.NES_20 == 2)
    {
        printf("    NES 2.0 file format unsupported\n");
        return;
    }

    mapper_number = header.flags_6.mapper_lo | (header.flags_7.mapper_hi << 4);

    printf("    Mapper: %u\n", mapper_number);

    if (!(mapper_number == 0 || mapper_number == 1 || mapper_number == 2))
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

    if (nes->mapper == MP_NROM) // Family basics prg ram
    {
        printf("    Allocating 8192 bytes of PRG RAM\n");
        if (nes->PRG_RAM == NULL)
            nes->PRG_RAM = (uint8_t*)malloc(8192);
        else
        {
            printf("    PRG RAM already allocated\n");
            free(nes->PRG_RAM);
            nes->PRG_RAM = (uint8_t*)malloc(8192);
        }
    }

    if (nes->mapper == MP_MMC1) // 32KB PRG RAM
    {
        printf("    Allocating 32768 bytes of PRG RAM\n");
        if (nes->PRG_RAM == NULL)
            nes->PRG_RAM = (uint8_t*)malloc(32768);
        else
        {
            printf("    PRG RAM already allocated\n");
            free(nes->PRG_RAM);
            nes->PRG_RAM = (uint8_t*)malloc(32768);
        }
    }

    nes_init_mmc1(nes);

    printf("    Mirroring: %s\n", mirroring_text[(uint8_t)mirroring]);
    printf("    Entry point: 0x%x\n", cpu_read_word(&nes->cpu, CPU_RESET_VECTOR));

    printf("Loading successful\n");
}