#pragma once

void cpu_reset(CPU* cpu)
{
    cpu->PC = cpu_read_word(cpu, CPU_RESET_VECTOR);
    cpu->S -= 3;
    cpu->P.I = 1;

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

    if (address < 0x4020)   // Unused for now
        return 0;

    switch (cpu->nes->mapper)
    {
    case MP_NROM:
        if (address < 0x6000)   
            return 0;
        if (address < 0x8000)   // Family Basic only: unused for now
            return 0;

        return cpu->nes->PRG_ROM_data[(address - 0x8000) % cpu->nes->PRG_ROM_size];

    default:
        return 0;
    }
}

void cpu_write_byte(CPU* cpu, uint16_t address, uint8_t value)
{
    // 2KB Internal RAM
    if (address < 0x2000)
        cpu->memory_low[address % 0x800] = value;

    switch (cpu->nes->mapper)
    {
    case MP_NROM:
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
    cpu_push_word(cpu, return_address);
    cpu_push_byte(cpu, *(uint8_t*)&cpu->P);

    cpu->PC = handler_address;

    cpu->cycle = 7;
}

uint16_t cpu_fetch_operands(CPU* cpu, CPU_INSTRUCTION instruction)
{
    switch(instruction.addressing_mode)
    {
    case AM_A:
        return 0;
    case AM_ABS:
        return cpu_read_word(cpu, cpu->PC + 1);
    case AM_ABS_X:
        return cpu_read_word(cpu, cpu->PC + 1) + cpu->X;
    case AM_ABS_Y:
        return cpu_read_word(cpu, cpu->PC + 1) + cpu->Y;
    case AM_IMM:
        return cpu->PC + 1;
    case AM_IMPL:
        return 0;
    case AM_IND:
        return cpu_read_word(cpu, cpu_read_word(cpu, cpu->PC + 1));
    case AM_X_IND:
        return cpu_read_word(cpu, (cpu_read_byte(cpu, cpu->PC + 1) + cpu->X) & 0xff);
    case AM_IND_Y:
        return cpu_read_word(cpu, cpu_read_byte(cpu, cpu->PC + 1)) + cpu->Y;
    case AM_REL:
        return cpu->PC + (int8_t)(cpu, cpu_read_byte(cpu, cpu->PC + 1));
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
        uint8_t opcode = cpu_read_byte(cpu, cpu->PC);
        CPU_INSTRUCTION instruction = cpu_instructions[opcode];
        uint16_t operand_address = cpu_fetch_operands(cpu, instruction);
        printf("0x%x | 0x%x : ", cpu->PC, opcode);
        if (instruction.instruction_handler == NULL)
        {
            printf("Invalid or illegal instruction");
            while(true);
        }
        (*instruction.instruction_handler)(cpu);
        cpu->PC += instruction_length[instruction.addressing_mode];

        printf("\n");
    }
    cpu->cycle--;
}

void CLI(CPU* cpu)
{
    printf("CLI");

    cpu->P.I = 0;

    cpu->cycle = 2;
}

void BRK(CPU* cpu)
{
    printf("BRK");

    cpu_throw_interrupt(cpu, cpu_read_word(cpu, CPU_BRK_VECTOR), cpu->PC + 2, true);
    cpu->PC--;
}