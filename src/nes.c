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

    nes.created = NES_CREATED_MAGIC_DWORD;
    nes.cpu.nes = &nes;

    return nes;
}

void nes_reset(NES* nes)
{
    if (nes->created != NES_CREATED_MAGIC_DWORD)
    {
        printf("NES object used while not initialized\n");
        return;
    }

    cpu_reset(&nes->cpu);
}

void nes_power_up(NES* nes)
{
    if (nes->created != NES_CREATED_MAGIC_DWORD)
    {
        printf("NES object used while not initialized\n");
        return;
    }
    
    cpu_power_up(&nes->cpu);

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
    nes->cycle_alignment %= 3;

    if (nes->cycle_alignment == 0)
    {
        cpu_cycle(&nes->cpu);
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
    uint8_t mapper_number = 0xff;

    FILE* f = fopen(path_to_rom, "rb");

    if (f == NULL)
        return;

    fread(&header, sizeof(header), 1, f);

    if (header.flags_6.trainer)
    {
        fseek(f, 512, SEEK_CUR);     // Skip the trainer
        printf("    Skipping trainer\n");
    }

    if (nes->PRG_ROM_data != NULL)
    {
        printf("    PRG ROM data already allocated\n");
        while(true);
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
        while(true);
        free(nes->CHR_ROM_data);
    }

    nes->CHR_ROM_size = 8192 * header.chr_rom;
    printf("    Allocating %u bytes of CHR ROM data\n", nes->CHR_ROM_size);
    nes->CHR_ROM_data = (uint8_t*)malloc(nes->CHR_ROM_size);

    if (nes->CHR_ROM_data == NULL)
    {
        printf("    Could'nt allocate data for the CHR ROM\n");
        return;
    }

    fread(nes->CHR_ROM_data, nes->CHR_ROM_size, 1, f);

    fclose(f);

    if (header.flags_7.NES_20 == 2)
    {
        printf("    NES 2.0 file format unsupported\n");
        while(true);
        return;
    }

    mapper_number = header.flags_6.mapper_lo | (header.flags_7.mapper_hi << 4);
    
    printf("    Mapper: %u\n", mapper_number);

    if (mapper_number > 0)
    {
        nes->mapper = MP_UNSUPPORTED;
        printf("Mapper unsupported\n", mapper_number);
        while(true);
        return;
    }
    else
        nes->mapper = (MAPPER)mapper_number;

    printf("Loading successful\n");
}