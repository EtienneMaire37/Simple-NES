#pragma once

void cpu_reset(CPU* cpu)
{
    cpu->PC = cpu_read_word(cpu, CPU_RESET_VECTOR);
    cpu->S -= 3;
    cpu->P.I = 1;
    cpu->dma = false;
    cpu->nmi = false;

    cpu->cycle = 7;     // Detailled behaviour at https://www.nesdev.org/wiki/CPU_interrupts
}

void cpu_power_up(CPU* cpu)
{
    cpu->A = cpu->X = cpu->Y = 0;
    cpu->S = 0;
    *(uint8_t*)&cpu->P = 0;
    cpu_reset(cpu);
}

uint8_t cpu_read_byte(CPU* cpu, uint16_t address)
{    
    // 2KB Internal RAM
    if (address < 0x2000)
        return cpu->memory_low[address % 0x800];

    if (address < 0x4020)   
    {
        if (address < 0x2008)   // PPU Registers
        {
            uint8_t tmp;
            switch (address)
            {
            case 0x2000:    // PPUCTRL
                return 0;
            case 0x2001:    // PPUMASK
                return 0;
            case 0x2002:    // PPUSTATUS
                cpu->nes->ppu.w = 0;
                tmp = *(uint8_t*)&cpu->nes->ppu.PPUSTATUS;
                cpu->nes->ppu.PPUSTATUS.vblank = 0;
                return tmp;
            case 0x2003:    // OAMADDR
                return 0;
            case 0x2004:    // OAMDATA
                return cpu->nes->ppu.oam_memory[cpu->nes->ppu.OAMADDR];
            case 0x2005:    // PPUSCROLL
                return 0;
            case 0x2006:    // PPUADDR
                return 0;
            case 0x2007:    // PPUDATA
                tmp = cpu->nes->ppu.last_read;
                cpu->nes->ppu.last_read = ppu_read_byte(&cpu->nes->ppu, *(uint16_t*)&cpu->nes->ppu.v);
                if (*(uint16_t*)&cpu->nes->ppu.v >= 0x3f00)  tmp = cpu->nes->ppu.last_read;
                *(uint16_t*)&cpu->nes->ppu.v += 1 + cpu->nes->ppu.PPUCTRL.address_increment * 31;
                return tmp;  
            }
        }

        if (address == 0x4014)  // OAM DMA
            return 0;

        if (address == 0x4016)  // Controller status
        {
            uint8_t tmp = (cpu->nes->key_status >> 7);
            cpu->nes->key_status <<= 1;
            cpu->nes->key_status |= 1;
            return (tmp & 1);
        }

        return 0;   // Unused for now
    }

    switch (cpu->nes->mapper)
    {
    case MP_NROM:
        if (address < 0x6000)   
            return 0;
        if (address < 0x8000)   // Family Basic only
            return cpu->nes->PRG_RAM[address - 0x6000];

        return cpu->nes->PRG_ROM_data[(address - 0x8000) % cpu->nes->PRG_ROM_size];

    case MP_MMC1:
        if (address < 0x6000)   
            return 0;

        if (address < 0x8000)   
            return cpu->nes->PRG_RAM[address - 0x6000 + cpu->nes->selected_prgram_bank * 0x2000];

        if ((cpu->nes->mmc1_control & 0b01000) == 0)     // PRG-ROM bank mode is 0 or 1 -> 32KB bankswitching
            return cpu->nes->PRG_ROM_data[(address - 0x8000 + (cpu->nes->selected_prgrom_bank_0 & 0b1110) * 0x4000) % cpu->nes->PRG_ROM_size];

        if (address < 0xc000)
        {
            if (((cpu->nes->mmc1_control & 0b01100) >> 2) == 0b10)   // First bank fixed second switchable
                return cpu->nes->PRG_ROM_data[(address - 0x8000) % cpu->nes->PRG_ROM_size];
            return cpu->nes->PRG_ROM_data[(address - 0xc000 + cpu->nes->selected_prgrom_bank_0 * 0x4000) % cpu->nes->PRG_ROM_size];
        }

        if (((cpu->nes->mmc1_control & 0b01100) >> 2) == 0b11)   // Second bank fixed first switchable
            return cpu->nes->PRG_ROM_data[((address - 0xc000) + cpu->nes->PRG_ROM_size - 0x4000) % cpu->nes->PRG_ROM_size];
        return cpu->nes->PRG_ROM_data[(address - 0xc000 + cpu->nes->selected_prgrom_bank_0 * 0x4000) % cpu->nes->PRG_ROM_size];

    case MP_UxROM:
        if (address < 0x8000)
            return 0;
        if (address < 0xc000)
            return cpu->nes->PRG_ROM_data[((address - 0x8000) + 0x4000 * cpu->nes->selected_prgrom_bank_0) % cpu->nes->PRG_ROM_size];
        return cpu->nes->PRG_ROM_data[((address - 0xc000) + cpu->nes->PRG_ROM_size - 0x4000) % cpu->nes->PRG_ROM_size];
    
    default:
        return 0;
    }
}

void cpu_write_byte(CPU* cpu, uint16_t address, uint8_t value)
{
    // 2KB Internal RAM
    if (address < 0x2000)
    {
        cpu->memory_low[address % 0x800] = value;
        return;
    }

    if (address < 0x4020)   
    {
        if (address < 0x2008)   // PPU Registers
        {
            switch (address)
            {
            case 0x2000:    // PPUCTRL
                *(uint8_t*)&cpu->nes->ppu.PPUCTRL = value;
                cpu->nes->ppu.t.nametable_select = (value & 0b11);
                return;
            case 0x2001:    // PPUMASK
                *(uint8_t*)&cpu->nes->ppu.PPUMASK = value;
                return;
            case 0x2002:    // PPUSTATUS
                return;
            case 0x2003:    // OAMADDR
                cpu->nes->ppu.OAMADDR = value;
                return;
            case 0x2004:    // OAMDATA
                cpu->nes->ppu.oam_memory[cpu->nes->ppu.OAMADDR] = value;
                cpu->nes->ppu.OAMADDR++;
                return;
            case 0x2005:    // PPUSCROLL
                if (cpu->nes->ppu.w == 0)
                {
                    cpu->nes->ppu.x = (value & 0b111);
                    cpu->nes->ppu.t.coarse_x = (value >> 3);
                }
                else
                {
                    cpu->nes->ppu.t.coarse_y = (value >> 3);
                    cpu->nes->ppu.t.fine_y = (value & 0b111);
                }
                cpu->nes->ppu.w ^= 1;
                return;
            case 0x2006:    // PPUADDR
                if (cpu->nes->ppu.w == 0)
                {
                    *(uint16_t*)&cpu->nes->ppu.t &= 0x00ff;
                    *(uint16_t*)&cpu->nes->ppu.t |= ((uint16_t)value & 0b00111111) << 8;
                }
                else
                {
                    *(uint16_t*)&cpu->nes->ppu.t &= 0xff00;
                    *(uint16_t*)&cpu->nes->ppu.t |= value;
                    cpu->nes->ppu.v = cpu->nes->ppu.t;
                }
                cpu->nes->ppu.w ^= 1;
                return;
            case 0x2007:    // PPUDATA
                ppu_write_byte(&cpu->nes->ppu, *(uint16_t*)&cpu->nes->ppu.v, value);  
                *(uint16_t*)&cpu->nes->ppu.v += 1 + cpu->nes->ppu.PPUCTRL.address_increment * 31;
                return;
            }
        }

        if (address == 0x4014)  // OAM DMA
        {
            for (uint16_t i = 0; i < 256; i++)
                cpu->nes->ppu.oam_memory[i] = cpu_read_byte(cpu, 0x100 * value + i);
            cpu->dma = true;
            return;
        }

        if (address == 0x4016)  // Controller strobe
        {
            cpu->nes->key_strobe = (value & 1);
            return;
        }

        return;   // Unused for now
    }

    switch (cpu->nes->mapper)
    {
    case MP_NROM:
        if (address < 0x6000)   
            return;
        if (address < 0x8000)   // Family Basic only
        {
            cpu->nes->PRG_RAM[address - 0x6000] = value; // PRG RAM
            return;   
        }
        break;

    case MP_MMC1:
        if (address < 0x6000)   
            return;

        cpu->nes->mmc1_shift_register >>= 1;
        cpu->nes->mmc1_shift_register |= ((value & 1) << 4);
        cpu->nes->mmc1_bits_shifted++;

        if (value >> 7)
        {
            cpu->nes->mmc1_shift_register = 0;
            cpu->nes->mmc1_bits_shifted = 0;
            cpu->nes->mmc1_control |= 0b01100;
            printf("MMC1 reset\n");
            return;
        }

        if (cpu->nes->mmc1_bits_shifted == 5)
        {
            cpu->nes->mmc1_bits_shifted = 0;

            if (address < 0xa000)   
            {
                cpu->nes->mmc1_control = cpu->nes->mmc1_shift_register;
                printf("MMC1 set control register 0x%x\n", cpu->nes->mmc1_shift_register);
                switch(cpu->nes->mmc1_control & 0b11)
                {
                case 0:
                    cpu->nes->ppu.mirroring = MR_ONESCREEN_LOWER;
                    break;
                case 1:
                    cpu->nes->ppu.mirroring = MR_ONESCREEN_HIGHER;
                    break;
                case 2:
                    cpu->nes->ppu.mirroring = MR_VERTICAL;
                    break;
                case 3:
                    cpu->nes->ppu.mirroring = MR_HORIZONTAL;
                    break;
                }
                return;
            }

            if (address < 0xc000)   
            {
                cpu->nes->selected_chrrom_bank_0 = cpu->nes->mmc1_shift_register;
                printf("MMC1 set chr rom bank 0 register 0x%x\n", cpu->nes->mmc1_shift_register);
                return;
            }

            if (address < 0xe000)
            {
                cpu->nes->selected_chrrom_bank_1 = cpu->nes->mmc1_shift_register;
                printf("MMC1 set chr rom bank 1 register 0x%x\n", cpu->nes->mmc1_shift_register);
                return;
            }

            cpu->nes->selected_prgrom_bank_0 = cpu->nes->mmc1_shift_register & 0b1111;
            printf("MMC1 set prg rom bank register 0x%x\n", cpu->nes->mmc1_shift_register);
            return;
        }

        return;
        
    case MP_UxROM:
        if (address < 0x8000)
            return;
        cpu->nes->selected_prgrom_bank_0 = (value & 0x0f) % (cpu->nes->PRG_ROM_size / 0x4000);
        break;
    }
}

uint16_t cpu_read_word(CPU* cpu, uint16_t address)
{
    // Little endian
    return cpu_read_byte(cpu, address) | ((uint16_t)cpu_read_byte(cpu, address + 1) << 8);
}

void cpu_write_word(CPU* cpu, uint16_t address, uint16_t value)
{
    // Little endian
    cpu_write_byte(cpu, address, (value & 0xff));
    cpu_write_byte(cpu, address + 1, (value >> 8));
}

void cpu_push_byte(CPU* cpu, uint8_t byte)
{
    cpu_write_byte(cpu, 0x0100 | cpu->S, byte);
    cpu->S--;
}

uint8_t cpu_pop_byte(CPU* cpu)
{
    cpu->S++;
    return cpu_read_byte(cpu, 0x0100 | cpu->S);
}

void cpu_push_word(CPU* cpu, uint16_t word)
{
    cpu_push_byte(cpu, word >> 8);
    cpu_push_byte(cpu, word & 0xff);
}

uint16_t cpu_pop_word(CPU* cpu)
{
    uint16_t word = cpu_pop_byte(cpu);
    return word | ((uint16_t)cpu_pop_byte(cpu) << 8);
}

void cpu_throw_interrupt(CPU* cpu, uint16_t handler_address, uint16_t return_address, bool b_flag)
{
    cpu->P.B = b_flag;
    cpu->P.reserved = 1;
    cpu_push_word(cpu, return_address);
    cpu_push_byte(cpu, *(uint8_t*)&cpu->P);

    cpu->PC = handler_address;

    cpu->cycle = 7;
}

uint16_t cpu_fetch_operands(CPU* cpu, CPU_INSTRUCTION instruction)
{
    uint16_t tmp, tmp1;
    uint8_t tmp8;
    switch(instruction.addressing_mode)
    {
    case AM_A:
        return 0;
    case AM_ABS:
        return cpu_read_word(cpu, cpu->PC + 1);
    case AM_ABS_X:
        tmp = cpu_read_word(cpu, cpu->PC + 1);
        cpu->page_boundary_crossed = ((tmp & 0xff00) != ((tmp + cpu->X) & 0xff00));
        return tmp + cpu->X;
    case AM_ABS_Y:
        tmp = cpu_read_word(cpu, cpu->PC + 1);
        cpu->page_boundary_crossed = ((tmp & 0xff00) != ((tmp + cpu->Y) & 0xff00));
        return tmp + cpu->Y;
    case AM_IMM:
        return cpu->PC + 1;
    case AM_IMPL:
        return 0;
    case AM_IND:
        tmp = cpu_read_word(cpu, cpu->PC + 1);
        return (cpu_read_byte(cpu, (((tmp + 1) & 0xff) | (tmp & 0xff00))) << 8) | cpu_read_byte(cpu, tmp);
    case AM_X_IND:
        tmp = ((cpu_read_byte(cpu, cpu->PC + 1) + cpu->X) & 0xff);
        return (cpu_read_byte(cpu, (((tmp + 1) & 0xff) | (tmp & 0xff00))) << 8) | cpu_read_byte(cpu, tmp);
    case AM_IND_Y:
        tmp1 = cpu_read_byte(cpu, cpu->PC + 1);
        tmp = cpu_read_byte(cpu, tmp1) + cpu_read_byte(cpu, ((tmp1 + 1) & 0xff)) * 256;
        cpu->page_boundary_crossed = ((tmp & 0xff00) != ((tmp + cpu->Y) & 0xff00));
        return tmp + cpu->Y;
    case AM_REL:
        tmp8 = cpu_read_byte(cpu, cpu->PC + 1);
        return cpu->PC + (int16_t)*(int8_t*)&tmp8;
    case AM_ZPG:
        return cpu_read_byte(cpu, cpu->PC + 1);
    case AM_ZPG_X:
        return (cpu_read_byte(cpu, cpu->PC + 1) + cpu->X) & 0xff;
    case AM_ZPG_Y:
        return (cpu_read_byte(cpu, cpu->PC + 1) + cpu->Y) & 0xff;
    default:
        ;
    }
}

void cpu_cycle(CPU* cpu)
{
    if (cpu->cycle == 0)
    {
        if (cpu->nmi)
        {
            cpu->nmi = false;
            cpu_throw_interrupt(cpu, cpu_read_word(cpu, CPU_NMI_VECTOR), cpu->PC, false);
        }
        else if (cpu->dma)
        {
            cpu->cycle = 513;
            cpu->dma = false;
        } 
        else
        {
            uint8_t opcode = cpu_read_byte(cpu, cpu->PC);
            CPU_INSTRUCTION instruction = cpu_instructions[opcode];
            cpu->operand_address = cpu_fetch_operands(cpu, instruction);
            LOG("0x%x | 0x%x : ", cpu->PC, opcode);
            if (instruction.instruction_handler == NULL)
            {
                LOG("Invalid or illegal instruction");
                return;
            }
            cpu->addressing_mode = instruction.addressing_mode;
            (*instruction.instruction_handler)(cpu);
            cpu->PC += instruction_length[instruction.addressing_mode];

            LOG(" | %s\n", addressing_mode_text[instruction.addressing_mode]);
        }
    }
    cpu->cycle--;
}

void BIT(CPU* cpu)
{
    LOG("BIT");

    uint8_t tmp = cpu_read_byte(cpu, cpu->operand_address);

    cpu->P.N = (tmp >> 7);
    cpu->P.V = (tmp >> 6) & 1;
    cpu->P.Z = ((tmp & cpu->A) == 0);

    if (cpu->addressing_mode == AM_ZPG)
        cpu->cycle = 3;
    else
        cpu->cycle = 4; // ABS
}

void CMP(CPU* cpu)
{
    LOG("CMP");

    uint8_t tmp = cpu_read_byte(cpu, cpu->operand_address);

    cpu->P.N = ((cpu->A - tmp) >> 7);
    cpu->P.Z = (tmp == cpu->A);
    cpu->P.C = (cpu->A >= tmp);

    switch (cpu->addressing_mode)
    {
    case AM_IMM:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ZPG_X:
        cpu->cycle = 4;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    case AM_ABS_X:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_ABS_Y:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_X_IND:
        cpu->cycle = 6;
        break;
    case AM_IND_Y:
        cpu->cycle = 5 + cpu->page_boundary_crossed;
        break;
    }
}

void CPX(CPU* cpu)
{
    LOG("CPX");

    uint8_t tmp = cpu_read_byte(cpu, cpu->operand_address);

    cpu->P.N = ((cpu->X - tmp) >> 7);
    cpu->P.Z = (tmp == cpu->X);
    cpu->P.C = (cpu->X >= tmp);

    switch (cpu->addressing_mode)
    {
    case AM_IMM:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    }
}

void CPY(CPU* cpu)
{
    LOG("CPY");

    uint8_t tmp = cpu_read_byte(cpu, cpu->operand_address);

    cpu->P.N = ((cpu->Y - tmp) >> 7);
    cpu->P.Z = (tmp == cpu->Y);
    cpu->P.C = (cpu->Y >= tmp);

    switch (cpu->addressing_mode)
    {
    case AM_IMM:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    }
}

void CLI(CPU* cpu)
{
    LOG("CLI");

    cpu->P.I = 0;

    cpu->cycle = 2;
}

void SEI(CPU* cpu)
{
    LOG("SEI");

    cpu->P.I = 1;

    cpu->cycle = 2;
}

void CLD(CPU* cpu)
{
    LOG("CLD");

    cpu->P.D = 0;

    cpu->cycle = 2;
}

void SED(CPU* cpu)
{
    LOG("SED");

    cpu->P.D = 1;

    cpu->cycle = 2;
}

void SEC(CPU* cpu)
{
    LOG("SEC");

    cpu->P.C = 1;

    cpu->cycle = 2;
}

void CLC(CPU* cpu)
{
    LOG("CLC");

    cpu->P.C = 0;

    cpu->cycle = 2;
}

void CLV(CPU* cpu)
{
    LOG("CLV");

    cpu->P.V = 0;

    cpu->cycle = 2;
}

void PHP(CPU* cpu)
{
    LOG("PHP");

    cpu->P.B = 1;
    cpu->P.reserved = 1;
    cpu_push_byte(cpu, *(uint8_t*)&cpu->P);

    cpu->cycle = 3;
}

void PLP(CPU* cpu)
{
    LOG("PLP");

    *(uint8_t*)&cpu->P = cpu_pop_byte(cpu);

    cpu->cycle = 4;
}

void PHA(CPU* cpu)
{
    LOG("PHA");

    cpu_push_byte(cpu, cpu->A);

    cpu->cycle = 3;
}

void PLA(CPU* cpu)
{
    LOG("PLA");

    cpu->A = cpu_pop_byte(cpu);

    cpu->P.N = (cpu->A >> 7);
    cpu->P.Z = (cpu->A == 0);

    cpu->cycle = 4;
}

void ORA(CPU* cpu)
{
    LOG("ORA");

    cpu->A |= cpu_read_byte(cpu, cpu->operand_address);

    cpu->P.N = (cpu->A >> 7);
    cpu->P.Z = (cpu->A == 0);

    switch(cpu->addressing_mode)
    {
    case AM_IMM:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ZPG_X:
        cpu->cycle = 4;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    case AM_ABS_X:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_ABS_Y:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_X_IND:
        cpu->cycle = 6;
        break;
    case AM_IND_Y:
        cpu->cycle = 5 + cpu->page_boundary_crossed;
        break;
    }
}

void AND(CPU* cpu)
{
    LOG("AND");

    cpu->A &= cpu_read_byte(cpu, cpu->operand_address);

    cpu->P.N = (cpu->A >> 7);
    cpu->P.Z = (cpu->A == 0);

    switch(cpu->addressing_mode)
    {
    case AM_IMM:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ZPG_X:
        cpu->cycle = 4;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    case AM_ABS_X:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_ABS_Y:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_X_IND:
        cpu->cycle = 6;
        break;
    case AM_IND_Y:
        cpu->cycle = 5 + cpu->page_boundary_crossed;
        break;
    }
}

void EOR(CPU* cpu)
{
    LOG("EOR");

    cpu->A ^= cpu_read_byte(cpu, cpu->operand_address);

    cpu->P.N = (cpu->A >> 7);
    cpu->P.Z = (cpu->A == 0);

    switch(cpu->addressing_mode)
    {
    case AM_IMM:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ZPG_X:
        cpu->cycle = 4;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    case AM_ABS_X:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_ABS_Y:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_X_IND:
        cpu->cycle = 6;
        break;
    case AM_IND_Y:
        cpu->cycle = 5 + cpu->page_boundary_crossed;
        break;
    }
}

void ASL(CPU* cpu)
{
    LOG("ASL");

    uint8_t tmp = (cpu->addressing_mode == AM_A ? cpu->A : cpu_read_byte(cpu, cpu->operand_address));

    cpu->P.C = (tmp >> 7);
    tmp <<= 1;

    cpu->P.N = (tmp >> 7);
    cpu->P.Z = (tmp == 0);

    if (cpu->addressing_mode == AM_A)
        cpu->A = tmp;
    else
        cpu_write_byte(cpu, cpu->operand_address, tmp);

    switch(cpu->addressing_mode)
    {
    case AM_A:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 5;
        break;
    case AM_ZPG_X:
        cpu->cycle = 6;
        break;
    case AM_ABS:
        cpu->cycle = 6;
        break;
    case AM_ABS_X:
        cpu->cycle = 7;
        break;
    }
}

void LSR(CPU* cpu)
{
    LOG("LSR");

    uint8_t tmp = (cpu->addressing_mode == AM_A ? cpu->A : cpu_read_byte(cpu, cpu->operand_address));

    cpu->P.C = (tmp & 1);
    tmp >>= 1;

    cpu->P.N = 0;
    cpu->P.Z = (tmp == 0);

    if (cpu->addressing_mode == AM_A)
        cpu->A = tmp;
    else
        cpu_write_byte(cpu, cpu->operand_address, tmp);

    switch(cpu->addressing_mode)
    {
    case AM_A:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 5;
        break;
    case AM_ZPG_X:
        cpu->cycle = 6;
        break;
    case AM_ABS:
        cpu->cycle = 6;
        break;
    case AM_ABS_X:
        cpu->cycle = 7;
        break;
    }
}

void ROL(CPU* cpu)
{
    LOG("ROL");

    uint8_t tmp = (cpu->addressing_mode == AM_A ? cpu->A : cpu_read_byte(cpu, cpu->operand_address));

    uint8_t tmp_c = cpu->P.C;
    cpu->P.C = (tmp >> 7);
    tmp <<= 1;
    tmp |= tmp_c;

    cpu->P.N = (tmp >> 7);
    cpu->P.Z = (tmp == 0);

    if (cpu->addressing_mode == AM_A)
        cpu->A = tmp;
    else
        cpu_write_byte(cpu, cpu->operand_address, tmp);

    switch(cpu->addressing_mode)
    {
    case AM_A:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 5;
        break;
    case AM_ZPG_X:
        cpu->cycle = 6;
        break;
    case AM_ABS:
        cpu->cycle = 6;
        break;
    case AM_ABS_X:
        cpu->cycle = 7;
        break;
    }
}

void ROR(CPU* cpu)
{
    LOG("ROR");

    uint8_t tmp = (cpu->addressing_mode == AM_A ? cpu->A : cpu_read_byte(cpu, cpu->operand_address));

    uint8_t tmp_c = cpu->P.C;
    cpu->P.C = (tmp & 1);
    tmp >>= 1;
    tmp |= (tmp_c << 7);

    cpu->P.N = (tmp >> 7);
    cpu->P.Z = (tmp == 0);

    if (cpu->addressing_mode == AM_A)
        cpu->A = tmp;
    else
        cpu_write_byte(cpu, cpu->operand_address, tmp);

    switch(cpu->addressing_mode)
    {
    case AM_A:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 5;
        break;
    case AM_ZPG_X:
        cpu->cycle = 6;
        break;
    case AM_ABS:
        cpu->cycle = 6;
        break;
    case AM_ABS_X:
        cpu->cycle = 7;
        break;
    }
}

void ADC(CPU* cpu)
{
    LOG("ADC");

    uint8_t value = cpu_read_byte(cpu, cpu->operand_address);
    uint16_t tmp = cpu->A + value + cpu->P.C;

    cpu->P.V = ((cpu->A ^ tmp) & (value ^ tmp)) >> 7;

    cpu->A = (uint8_t)tmp;

    cpu->P.N = (cpu->A >> 7);
    cpu->P.Z = (cpu->A == 0);
    cpu->P.C = (tmp >> 8) & 1;

    switch(cpu->addressing_mode)
    {
    case AM_IMM:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ZPG_X:
        cpu->cycle = 4;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    case AM_ABS_X:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_ABS_Y:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_X_IND:
        cpu->cycle = 6;
        break;
    case AM_IND_Y:
        cpu->cycle = 5 + cpu->page_boundary_crossed;
        break;
    }
}

void SBC(CPU* cpu)
{
    LOG("SBC");

    uint8_t value = cpu_read_byte(cpu, cpu->operand_address);
    int16_t tmp = cpu->A - value - 1 + cpu->P.C;

    cpu->P.V = ((cpu->A ^ value) & 0x80) != 0 && ((cpu->A ^ (uint8_t)tmp) & 0x80) != 0;

    cpu->A = (uint8_t)tmp;

    cpu->P.N = (cpu->A >> 7);
    cpu->P.Z = (cpu->A == 0);
    cpu->P.C = tmp >= 0;

    switch(cpu->addressing_mode)
    {
    case AM_IMM:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ZPG_X:
        cpu->cycle = 4;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    case AM_ABS_X:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_ABS_Y:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_X_IND:
        cpu->cycle = 6;
        break;
    case AM_IND_Y:
        cpu->cycle = 5 + cpu->page_boundary_crossed;
        break;
    }
}

void DEC(CPU* cpu)
{
    LOG("DEC");

    uint8_t tmp = cpu_read_byte(cpu, cpu->operand_address) - 1;

    cpu_write_byte(cpu, cpu->operand_address, tmp);

    cpu->P.N = (tmp >> 7);
    cpu->P.Z = (tmp == 0);

    switch (cpu->addressing_mode)
    {
    case AM_ZPG:
        cpu->cycle = 5;
        break;
    case AM_ZPG_X:
        cpu->cycle = 6;
        break;
    case AM_ABS:
        cpu->cycle = 6;
        break;
    case AM_ABS_X:
        cpu->cycle = 7;
        break;
    }
}

void INC(CPU* cpu)
{
    LOG("INC");

    uint8_t tmp = cpu_read_byte(cpu, cpu->operand_address) + 1;

    cpu_write_byte(cpu, cpu->operand_address, tmp);

    cpu->P.N = (tmp >> 7);
    cpu->P.Z = (tmp == 0);

    switch (cpu->addressing_mode)
    {
    case AM_ZPG:
        cpu->cycle = 5;
        break;
    case AM_ZPG_X:
        cpu->cycle = 6;
        break;
    case AM_ABS:
        cpu->cycle = 6;
        break;
    case AM_ABS_X:
        cpu->cycle = 7;
        break;
    }
}

void DEY(CPU* cpu)
{
    LOG("DEY");

    cpu->Y--;

    cpu->P.N = (cpu->Y >> 7);
    cpu->P.Z = (cpu->Y == 0);

    cpu->cycle = 2;
}

void INY(CPU* cpu)
{
    LOG("INY");

    cpu->Y++;

    cpu->P.N = (cpu->Y >> 7);
    cpu->P.Z = (cpu->Y == 0);

    cpu->cycle = 2;
}

void DEX(CPU* cpu)
{
    LOG("DEX");

    cpu->X--;

    cpu->P.N = (cpu->X >> 7);
    cpu->P.Z = (cpu->X == 0);

    cpu->cycle = 2;
}

void INX(CPU* cpu)
{
    LOG("INX");

    cpu->X++;

    cpu->P.N = (cpu->X >> 7);
    cpu->P.Z = (cpu->X == 0);

    cpu->cycle = 2;
}

void LDA(CPU* cpu)
{
    LOG("LDA");

    cpu->A = cpu_read_byte(cpu, cpu->operand_address);

    cpu->P.N = (cpu->A >> 7);
    cpu->P.Z = (cpu->A == 0);

    switch(cpu->addressing_mode)
    {
    case AM_IMM:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ZPG_X:
        cpu->cycle = 4;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    case AM_ABS_X:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_ABS_Y:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    case AM_X_IND:
        cpu->cycle = 6;
        break;
    case AM_IND_Y:
        cpu->cycle = 5 + cpu->page_boundary_crossed;
        break;
    }
}

void LDX(CPU* cpu)
{
    LOG("LDX");

    cpu->X = cpu_read_byte(cpu, cpu->operand_address);

    cpu->P.N = (cpu->X >> 7);
    cpu->P.Z = (cpu->X == 0);

    switch(cpu->addressing_mode)
    {
    case AM_IMM:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ZPG_Y:
        cpu->cycle = 4;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    case AM_ABS_Y:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    }
}

void LDY(CPU* cpu)
{
    LOG("LDY");

    cpu->Y = cpu_read_byte(cpu, cpu->operand_address);

    cpu->P.N = (cpu->Y >> 7);
    cpu->P.Z = (cpu->Y == 0);

    switch(cpu->addressing_mode)
    {
    case AM_IMM:
        cpu->cycle = 2;
        break;
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ZPG_X:
        cpu->cycle = 4;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    case AM_ABS_X:
        cpu->cycle = 4 + cpu->page_boundary_crossed;
        break;
    }
}

void STA(CPU* cpu)
{
    LOG("STA");

    cpu_write_byte(cpu, cpu->operand_address, cpu->A);

    switch(cpu->addressing_mode)
    {
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ZPG_X:
        cpu->cycle = 4;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    case AM_ABS_X:
        cpu->cycle = 5;
        break;
    case AM_ABS_Y:
        cpu->cycle = 5;
        break;
    case AM_X_IND:
        cpu->cycle = 6;
        break;
    case AM_IND_Y:
        cpu->cycle = 6;
        break;
    }
}

void STX(CPU* cpu)
{
    LOG("STX");

    cpu_write_byte(cpu, cpu->operand_address, cpu->X);

    switch(cpu->addressing_mode)
    {
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ZPG_Y:
        cpu->cycle = 4;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    }
}

void STY(CPU* cpu)
{
    LOG("STY");

    cpu_write_byte(cpu, cpu->operand_address, cpu->Y);

    switch(cpu->addressing_mode)
    {
    case AM_ZPG:
        cpu->cycle = 3;
        break;
    case AM_ZPG_X:
        cpu->cycle = 4;
        break;
    case AM_ABS:
        cpu->cycle = 4;
        break;
    }
}

void TXA(CPU* cpu)
{
    LOG("TXA");

    cpu->A = cpu->X;

    cpu->P.N = (cpu->A >> 7);
    cpu->P.Z = (cpu->A == 0);

    cpu->cycle = 2;
}

void TAX(CPU* cpu)
{
    LOG("TAX");

    cpu->X = cpu->A;

    cpu->P.N = (cpu->X >> 7);
    cpu->P.Z = (cpu->X == 0);

    cpu->cycle = 2;
}

void TYA(CPU* cpu)
{
    LOG("TYA");

    cpu->A = cpu->Y;

    cpu->P.N = (cpu->A >> 7);
    cpu->P.Z = (cpu->A == 0);

    cpu->cycle = 2;
}

void TAY(CPU* cpu)
{
    LOG("TAY");

    cpu->Y = cpu->A;

    cpu->P.N = (cpu->Y >> 7);
    cpu->P.Z = (cpu->Y == 0);

    cpu->cycle = 2;
}

void TXS(CPU* cpu)
{
    LOG("TXS");

    cpu->S = cpu->X;

    cpu->cycle = 2;
}

void TSX(CPU* cpu)
{
    LOG("TSX");

    cpu->X = cpu->S;

    cpu->P.N = (cpu->X >> 7);
    cpu->P.Z = (cpu->X == 0);

    cpu->cycle = 2;
}

void BPL(CPU* cpu)
{
    LOG("BPL");

    cpu->cycle = 2;

    if (!cpu->P.N)
    {
        cpu->cycle++;
        if (((cpu->PC + 2) & 0xff00) != ((cpu->operand_address + 2) & 0xff00))
            cpu->cycle++;
        cpu->PC = cpu->operand_address;
    }
}

void BMI(CPU* cpu)
{
    LOG("BMI");

    cpu->cycle = 2;

    if (cpu->P.N)
    {
        cpu->cycle++;
        if (((cpu->PC + 2) & 0xff00) != ((cpu->operand_address + 2) & 0xff00))
            cpu->cycle++;
        cpu->PC = cpu->operand_address;
    }
}

void BVC(CPU* cpu)
{
    LOG("BVC");

    cpu->cycle = 2;

    if (!cpu->P.V)
    {
        cpu->cycle++;
        if (((cpu->PC + 2) & 0xff00) != ((cpu->operand_address + 2) & 0xff00))
            cpu->cycle++;
        cpu->PC = cpu->operand_address;
    }
}

void BVS(CPU* cpu)
{
    LOG("BVS");

    cpu->cycle = 2;

    if (cpu->P.V)
    {
        cpu->cycle++;
        if (((cpu->PC + 2) & 0xff00) != ((cpu->operand_address + 2) & 0xff00))
            cpu->cycle++;
        cpu->PC = cpu->operand_address;
    }
}

void BCC(CPU* cpu)
{
    LOG("BCC");

    cpu->cycle = 2;

    if (!cpu->P.C)
    {
        cpu->cycle++;
        if (((cpu->PC + 2) & 0xff00) != ((cpu->operand_address + 2) & 0xff00))
            cpu->cycle++;
        cpu->PC = cpu->operand_address;
    }
}

void BCS(CPU* cpu)
{
    LOG("BCS");

    cpu->cycle = 2;

    if (cpu->P.C)
    {
        cpu->cycle++;
        if (((cpu->PC + 2) & 0xff00) != ((cpu->operand_address + 2) & 0xff00))
            cpu->cycle++;
        cpu->PC = cpu->operand_address;
    }
}

void BNE(CPU* cpu)
{
    LOG("BNE");

    cpu->cycle = 2;

    if (!cpu->P.Z)
    {
        cpu->cycle++;
        if (((cpu->PC + 2) & 0xff00) != ((cpu->operand_address + 2) & 0xff00))
            cpu->cycle++;
        cpu->PC = cpu->operand_address;
    }
}

void BEQ(CPU* cpu)
{
    LOG("BEQ");

    cpu->cycle = 2;

    if (cpu->P.Z)
    {
        cpu->cycle++;
        if (((cpu->PC + 2) & 0xff00) != ((cpu->operand_address + 2) & 0xff00))
            cpu->cycle++;
        cpu->PC = cpu->operand_address;
    }
}

void JMP(CPU* cpu)
{
    LOG("JMP");

    cpu->PC = cpu->operand_address;
    cpu->PC -= 3;

    if (cpu->addressing_mode == AM_ABS)
        cpu->cycle = 3;
    else
        cpu->cycle = 5; // IND
}

void JSR(CPU* cpu)
{
    LOG("JSR");

    cpu_push_word(cpu, cpu->PC + 2);    // Last byte of JSR because RTS will offset the return address

    cpu->PC = cpu->operand_address - 3;

    cpu->cycle = 6;
}

void RTS(CPU* cpu)
{
    LOG("RTS");

    cpu->PC = cpu_pop_word(cpu);

    cpu->cycle = 6;
}

void BRK(CPU* cpu)
{
    LOG("BRK");

    cpu_throw_interrupt(cpu, cpu_read_word(cpu, CPU_BRK_VECTOR), cpu->PC + 2, true);
    cpu->PC--;
}

void RTI(CPU* cpu)
{
    LOG("RTI");

    *(uint8_t*)&cpu->P = cpu_pop_byte(cpu);
    cpu->PC = cpu_pop_word(cpu) - 1;

    cpu->cycle = 6;
}

void NOP(CPU* cpu)
{
    LOG("NOP");

    cpu->cycle = 2;
}