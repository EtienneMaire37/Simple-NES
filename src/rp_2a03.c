#pragma once

void cpu_reset(CPU* cpu)
{
    cpu->PC = cpu_read_word(cpu, CPU_RESET_VECTOR);
    cpu->S -= 3;
    cpu->P.I = 1;

    cpu->cycle = 6;
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

    return 0;
}

void cpu_write_byte(CPU* cpu, uint16_t address, uint8_t value)
{
    // 2KB Internal RAM
    if (address < 0x2000)
        cpu->memory_low[address % 0x800] = value;
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
        printf("0x%x : ", opcode);
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