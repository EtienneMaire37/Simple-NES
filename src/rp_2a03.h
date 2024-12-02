#pragma once

#define CPU_NMI_VECTOR      0xFFFA
#define CPU_RESET_VECTOR    0xFFFC
#define CPU_IRQ_VECTOR      0xFFFE
#define CPU_BRK_VECTOR      0xFFFE

typedef struct NES NES;

typedef enum CPU_ADDRESSING_MODE
{
    AM_A = 0,
    AM_ABS = 1,
    AM_ABS_X = 2,
    AM_ABS_Y = 3,
    AM_IMM = 4,
    AM_IMPL = 5,
    AM_IND = 6,
    AM_X_IND = 7,
    AM_IND_Y = 8,
    AM_REL = 9,
    AM_ZPG = 10,
    AM_ZPG_X = 11,
    AM_ZPG_Y = 12
} CPU_ADDRESSING_MODE;

typedef struct RP_2A03_FLAGS
{
    uint8_t C   : 1;    // Carry
    uint8_t Z   : 1;    // Zero
    uint8_t I   : 1;    // Interrupt disable
    uint8_t D   : 1;    // Decimal mode ; Not supported by the 2A03

    uint8_t B   : 1;        // Break flag
    uint8_t reserved : 1;   // Always pushed as 1
    
    uint8_t V   : 1;    // Overflow
    uint8_t N   : 1;    // Negative
} CPU_FLAGS;

typedef struct RP_2A03
{
    uint8_t memory_low[0x800];  // $0000-$07FF

    // Registers
    uint8_t A, X, Y;    // Accumulator, X index, Y index
    uint16_t PC;        // Program counter
    uint8_t S;          // Stack pointer
    CPU_FLAGS P;        // Flags

    uint16_t cycle;     // How many cycles the cpu needs to execute to finish the current instruction

    // Effective address of instruction / address of operand
    uint16_t operand_address;
    CPU_ADDRESSING_MODE addressing_mode; // Addressing mode of current instruction
    uint8_t page_boundary_crossed;  // 0 or 1 | https://www.masswerk.at/6502/6502_instruction_set.html#opcodes-footnote1

    NES* nes;
} CPU;

uint8_t instruction_length[13] = 
{
    1,
    3,
    3,
    3,
    2,
    1,
    3,
    2,
    2,
    2,
    2,
    2,
    2
};

typedef struct CPU_INSTRUCTION
{
    void (*instruction_handler)(CPU* cpu);
    CPU_ADDRESSING_MODE addressing_mode;
} CPU_INSTRUCTION;

char* addressing_mode_text[13] = 
{
    "Accumulator",
    "Absolute",
    "Absolute, X",
    "Absolute, Y",
    "Immediate",
    "Implied",
    "(Indirect)",
    "(X, Indirect)",
    "(Indirect), Y",
    "Relative",
    "Zeropage",
    "Zeropage, X",
    "Zeropage, Y"
};

void cpu_reset(CPU* cpu);
void cpu_power_up(CPU* cpu);
uint8_t cpu_read_byte(CPU* cpu, uint16_t address);
void cpu_write_byte(CPU* cpu, uint16_t address, uint8_t value);
uint16_t cpu_read_word(CPU* cpu, uint16_t address);
void cpu_write_word(CPU* cpu, uint16_t address, uint16_t value);

void BIT(CPU* cpu);

void CLI(CPU* cpu);
void SEI(CPU* cpu);

void CLD(CPU* cpu);

void CLC(CPU* cpu);

void PHP(CPU* cpu);
void PLP(CPU* cpu);

void ORA(CPU* cpu);
void AND(CPU* cpu);
void ASL(CPU* cpu);
void ROL(CPU* cpu);

void BPL(CPU* cpu);

void JSR(CPU* cpu);

void BRK(CPU* cpu);

CPU_INSTRUCTION cpu_instructions[256] = 
{
    { &BRK, AM_IMPL },  { &ORA, AM_X_IND }, { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { &ORA, AM_ZPG },    { &ASL, AM_ZPG },   { NULL, 0 }, { &PHP, AM_IMPL },      { &ORA, AM_IMM },     { &ASL, AM_A },     { NULL, 0 }, { NULL, 0 },       { &ORA, AM_ABS },    { &ASL, AM_ABS },     { NULL, 0 }, 
    { &BPL, AM_REL },   { &ORA, AM_IND_Y }, { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { &ORA, AM_ZPG_X },  { &ASL, AM_ZPG_X }, { NULL, 0 }, { &CLC, AM_IMPL },      { &ORA, AM_ABS_Y },   { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { &ORA, AM_ABS_X },  { &ASL, AM_ABS_X },   { NULL, 0 }, 
    { &JSR, AM_ABS },   { &AND, AM_X_IND }, { NULL, 0 }, { NULL, 0 }, { &BIT, AM_ZPG },     { &AND, AM_ZPG },    { &ROL, AM_ZPG },   { NULL, 0 }, { &PLP, AM_IMPL },      { &AND, AM_IMM },     { &ROL, AM_A },     { NULL, 0 }, { &BIT, AM_ABS },  { &AND, AM_ABS },    { &ROL, AM_ABS },     { NULL, 0 }, 
    
    { NULL, 0 },        { &AND, AM_IND_Y }, { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { &AND, AM_ZPG_X },  { &ROL, AM_ZPG_X }, { NULL, 0 }, { NULL, 0 },            { &AND, AM_ABS_Y },   { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { &AND, AM_ABS_X },  { &ROL, AM_ABS_X },   { NULL, 0 }, 
    
    { NULL, 0 },        { NULL, 0 },        { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { NULL, 0 },         { NULL, 0 },        { NULL, 0 }, { NULL, 0 },            { NULL, 0 },          { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { NULL, 0 },         { NULL, 0 },          { NULL, 0 }, 
    { NULL, 0 },        { NULL, 0 },        { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { NULL, 0 },         { NULL, 0 },        { NULL, 0 }, { &CLI, AM_IMPL },      { NULL, 0 },          { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { NULL, 0 },         { NULL, 0 },          { NULL, 0 }, 
    { NULL, 0 },        { NULL, 0 },        { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { NULL, 0 },         { NULL, 0 },        { NULL, 0 }, { NULL, 0 },            { NULL, 0 },          { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { NULL, 0 },         { NULL, 0 },          { NULL, 0 }, 
    { NULL, 0 },        { NULL, 0 },        { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { NULL, 0 },         { NULL, 0 },        { NULL, 0 }, { &SEI, AM_IMPL },      { NULL, 0 },          { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { NULL, 0 },         { NULL, 0 },          { NULL, 0 }, 
    { NULL, 0 },        { NULL, 0 },        { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { NULL, 0 },         { NULL, 0 },        { NULL, 0 }, { NULL, 0 },            { NULL, 0 },          { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { NULL, 0 },         { NULL, 0 },          { NULL, 0 }, 
    { NULL, 0 },        { NULL, 0 },        { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { NULL, 0 },         { NULL, 0 },        { NULL, 0 }, { NULL, 0 },            { NULL, 0 },          { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { NULL, 0 },         { NULL, 0 },          { NULL, 0 }, 
    { NULL, 0 },        { NULL, 0 },        { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { NULL, 0 },         { NULL, 0 },        { NULL, 0 }, { NULL, 0 },            { NULL, 0 },          { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { NULL, 0 },         { NULL, 0 },          { NULL, 0 }, 
    { NULL, 0 },        { NULL, 0 },        { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { NULL, 0 },         { NULL, 0 },        { NULL, 0 }, { NULL, 0 },            { NULL, 0 },          { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { NULL, 0 },         { NULL, 0 },          { NULL, 0 }, 
    { NULL, 0 },        { NULL, 0 },        { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { NULL, 0 },         { NULL, 0 },        { NULL, 0 }, { NULL, 0 },            { NULL, 0 },          { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { NULL, 0 },         { NULL, 0 },          { NULL, 0 }, 
    { NULL, 0 },        { NULL, 0 },        { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { NULL, 0 },         { NULL, 0 },        { NULL, 0 }, { &CLD, AM_IMPL },      { NULL, 0 },          { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { NULL, 0 },         { NULL, 0 },          { NULL, 0 }, 
    { NULL, 0 },        { NULL, 0 },        { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { NULL, 0 },         { NULL, 0 },        { NULL, 0 }, { NULL, 0 },            { NULL, 0 },          { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { NULL, 0 },         { NULL, 0 },          { NULL, 0 }, 
    { NULL, 0 },        { NULL, 0 },        { NULL, 0 }, { NULL, 0 }, { NULL, 0 },          { NULL, 0 },         { NULL, 0 },        { NULL, 0 }, { NULL, 0 },            { NULL, 0 },          { NULL, 0 },        { NULL, 0 }, { NULL, 0 },       { NULL, 0 },         { NULL, 0 },          { NULL, 0 }
};