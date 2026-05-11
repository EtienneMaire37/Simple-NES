#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct NES NES;

#define CPU_NMI_VECTOR      0xFFFA
#define CPU_RESET_VECTOR    0xFFFC
#define CPU_IRQ_VECTOR      0xFFFE
#define CPU_BRK_VECTOR      0xFFFE

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
    AM_ZPG_Y = 12,
    AM_UK = -1
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

typedef struct RP_2A03_CPU
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

    bool nmi, nmi_last_requested_state, nmi_requested, nmi_last_requested;
    uint8_t nmi_latch;
    bool dma;
    uint8_t dma_page;
    uint8_t dma_counter;

    uint32_t apu_counter;

    NES* nes;
} CPU;

static const uint8_t instruction_length[13] =
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

static const char* addressing_mode_text[13] =
{
    "Accumulator",
    "Absolute",
    "Absolute, X",
    "Absolute, Y",
    "Immediate",
    "Implied",
    "(Indirect)",
    "(Indirect, X)",
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
void cpu_cycle(CPU* cpu);

void BIT(CPU* cpu);
void CMP(CPU* cpu);
void CPX(CPU* cpu);
void CPY(CPU* cpu);

void CLI(CPU* cpu);
void SEI(CPU* cpu);
void CLD(CPU* cpu);
void SED(CPU* cpu);
void CLC(CPU* cpu);
void SEC(CPU* cpu);
void CLV(CPU* cpu);

void PHP(CPU* cpu);
void PLP(CPU* cpu);

void PHA(CPU* cpu);
void PLA(CPU* cpu);

void ORA(CPU* cpu);
void AND(CPU* cpu);
void EOR(CPU* cpu);

void ASL(CPU* cpu);
void LSR(CPU* cpu);

void ROL(CPU* cpu);
void ROR(CPU* cpu);

void ADC(CPU* cpu);
void SBC(CPU* cpu);

void INC(CPU* cpu);
void DEC(CPU* cpu);

void DEY(CPU* cpu);
void INY(CPU* cpu);
void DEX(CPU* cpu);
void INX(CPU* cpu);

void LDA(CPU* cpu);
void LDX(CPU* cpu);
void LDY(CPU* cpu);

void STA(CPU* cpu);
void STX(CPU* cpu);
void STY(CPU* cpu);

void TXA(CPU* cpu);
void TAX(CPU* cpu);
void TYA(CPU* cpu);
void TAY(CPU* cpu);
void TXS(CPU* cpu);
void TSX(CPU* cpu);

void BPL(CPU* cpu);
void BMI(CPU* cpu);
void BVC(CPU* cpu);
void BVS(CPU* cpu);
void BCC(CPU* cpu);
void BCS(CPU* cpu);
void BNE(CPU* cpu);
void BEQ(CPU* cpu);

void JMP(CPU* cpu);

void JSR(CPU* cpu);
void RTS(CPU* cpu);

void BRK(CPU* cpu);
void RTI(CPU* cpu);

void NOP(CPU* cpu);

static const CPU_INSTRUCTION cpu_instructions[256] =
{
            /*     0xX0        */  /*     0xX1      */ /* 0xX2      */  /* 0xX3  */  /*     0xX4      */   /*     0xX5      */  /*     0xX6      */ /* 0xX7  */  /*     0xX8      */     /*     0xX9      */   /*     0xXA      */ /* 0xXB  */  /*    0xXC     */  /*     0xXD      */  /*     0xXE      */   /* 0xXF  */
/* 0x0X  */    { &BRK, AM_IMPL },  { &ORA, AM_X_IND }, { NULL, AM_UK },  { NULL, AM_UK }, { NULL, AM_UK },       { &ORA, AM_ZPG },    { &ASL, AM_ZPG },   { NULL, AM_UK }, { &PHP, AM_IMPL },      { &ORA, AM_IMM },     { &ASL, AM_A },     { NULL, AM_UK }, { NULL, AM_UK },   { &ORA, AM_ABS },    { &ASL, AM_ABS },     { NULL, AM_UK },
/* 0x1X  */    { &BPL, AM_REL },   { &ORA, AM_IND_Y }, { NULL, AM_UK },  { NULL, AM_UK }, { NULL, AM_UK },       { &ORA, AM_ZPG_X },  { &ASL, AM_ZPG_X }, { NULL, AM_UK }, { &CLC, AM_IMPL },      { &ORA, AM_ABS_Y },   { NULL, AM_UK },    { NULL, AM_UK }, { NULL, AM_UK },   { &ORA, AM_ABS_X },  { &ASL, AM_ABS_X },   { NULL, AM_UK },
/* 0x2X  */    { &JSR, AM_ABS },   { &AND, AM_X_IND }, { NULL, AM_UK },  { NULL, AM_UK }, { &BIT, AM_ZPG },      { &AND, AM_ZPG },    { &ROL, AM_ZPG },   { NULL, AM_UK }, { &PLP, AM_IMPL },      { &AND, AM_IMM },     { &ROL, AM_A },     { NULL, AM_UK }, { &BIT, AM_ABS },  { &AND, AM_ABS },    { &ROL, AM_ABS },     { NULL, AM_UK },
/* 0x3X  */    { &BMI, AM_REL },   { &AND, AM_IND_Y }, { NULL, AM_UK },  { NULL, AM_UK }, { NULL, AM_UK },       { &AND, AM_ZPG_X },  { &ROL, AM_ZPG_X }, { NULL, AM_UK }, { &SEC, AM_IMPL },      { &AND, AM_ABS_Y },   { NULL, AM_UK },    { NULL, AM_UK }, { NULL, AM_UK },   { &AND, AM_ABS_X },  { &ROL, AM_ABS_X },   { NULL, AM_UK },
/* 0x4X  */    { &RTI, AM_IMPL },  { &EOR, AM_X_IND }, { NULL, AM_UK },  { NULL, AM_UK }, { NULL, AM_UK },       { &EOR, AM_ZPG },    { &LSR, AM_ZPG },   { NULL, AM_UK }, { &PHA, AM_IMPL },      { &EOR, AM_IMM },     { &LSR, AM_A },     { NULL, AM_UK }, { &JMP, AM_ABS },  { &EOR, AM_ABS },    { &LSR, AM_ABS },     { NULL, AM_UK },
/* 0x5X  */    { &BVC, AM_REL },   { &EOR, AM_IND_Y }, { NULL, AM_UK },  { NULL, AM_UK }, { NULL, AM_UK },       { &EOR, AM_ZPG_X },  { &LSR, AM_ZPG_X }, { NULL, AM_UK }, { &CLI, AM_IMPL },      { &EOR, AM_ABS_Y },   { NULL, AM_UK },    { NULL, AM_UK }, { NULL, AM_UK },   { &EOR, AM_ABS_X },  { &LSR, AM_ABS_X },   { NULL, AM_UK },
/* 0x6X  */    { &RTS, AM_IMPL },  { &ADC, AM_X_IND }, { NULL, AM_UK },  { NULL, AM_UK }, { NULL, AM_UK },       { &ADC, AM_ZPG },    { &ROR, AM_ZPG },   { NULL, AM_UK }, { &PLA, AM_IMPL },      { &ADC, AM_IMM },     { &ROR, AM_A },     { NULL, AM_UK }, { &JMP, AM_IND },  { &ADC, AM_ABS },    { &ROR, AM_ABS },     { NULL, AM_UK },
/* 0x7X  */    { &BVS, AM_REL },   { &ADC, AM_IND_Y }, { NULL, AM_UK },  { NULL, AM_UK }, { NULL, AM_UK },       { &ADC, AM_ZPG_X },  { &ROR, AM_ZPG_X }, { NULL, AM_UK }, { &SEI, AM_IMPL },      { &ADC, AM_ABS_Y },   { NULL, AM_UK },    { NULL, AM_UK }, { NULL, AM_UK },   { &ADC, AM_ABS_X },  { &ROR, AM_ABS_X },   { NULL, AM_UK },
/* 0x8X  */    { NULL, AM_UK },    { &STA, AM_X_IND }, { NULL, AM_UK },  { NULL, AM_UK }, { &STY, AM_ZPG },      { &STA, AM_ZPG },    { &STX, AM_ZPG },   { NULL, AM_UK }, { &DEY, AM_IMPL },      { NULL, AM_UK },      { &TXA, AM_IMPL },  { NULL, AM_UK }, { &STY, AM_ABS },  { &STA, AM_ABS },    { &STX, AM_ABS },     { NULL, AM_UK },
/* 0x9X  */    { &BCC, AM_REL },   { &STA, AM_IND_Y }, { NULL, AM_UK },  { NULL, AM_UK }, { &STY, AM_ZPG_X },    { &STA, AM_ZPG_X },  { &STX, AM_ZPG_Y }, { NULL, AM_UK }, { &TYA, AM_IMPL },      { &STA, AM_ABS_Y },   { &TXS, AM_IMPL },  { NULL, AM_UK }, { NULL, AM_UK },   { &STA, AM_ABS_X },  { NULL, AM_UK },      { NULL, AM_UK },
/* 0xAX  */    { &LDY, AM_IMM },   { &LDA, AM_X_IND }, { &LDX, AM_IMM }, { NULL, AM_UK }, { &LDY, AM_ZPG },      { &LDA, AM_ZPG },    { &LDX, AM_ZPG },   { NULL, AM_UK }, { &TAY, AM_IMPL },      { &LDA, AM_IMM },     { &TAX, AM_IMPL },  { NULL, AM_UK }, { &LDY, AM_ABS },  { &LDA, AM_ABS },    { &LDX, AM_ABS },     { NULL, AM_UK },
/* 0xBX  */    { &BCS, AM_REL },   { &LDA, AM_IND_Y }, { NULL, AM_UK },  { NULL, AM_UK }, { &LDY, AM_ZPG_X },    { &LDA, AM_ZPG_X },  { &LDX, AM_ZPG_Y }, { NULL, AM_UK }, { &CLV, AM_IMPL },      { &LDA, AM_ABS_Y },   { &TSX, AM_IMPL },  { NULL, AM_UK }, { &LDY, AM_ABS_X },{ &LDA, AM_ABS_X },  { &LDX, AM_ABS_Y },   { NULL, AM_UK },
/* 0xCX  */    { &CPY, AM_IMM },   { &CMP, AM_X_IND }, { NULL, AM_UK },  { NULL, AM_UK }, { &CPY, AM_ZPG },      { &CMP, AM_ZPG },    { &DEC, AM_ZPG },   { NULL, AM_UK }, { &INY, AM_IMPL },      { &CMP, AM_IMM },     { &DEX, AM_IMPL },  { NULL, AM_UK }, { &CPY, AM_ABS },  { &CMP, AM_ABS },    { &DEC, AM_ABS },     { NULL, AM_UK },
/* 0xDX  */    { &BNE, AM_REL },   { &CMP, AM_IND_Y }, { NULL, AM_UK },  { NULL, AM_UK }, { NULL, AM_UK },       { &CMP, AM_ZPG_X },  { &DEC, AM_ZPG_X }, { NULL, AM_UK }, { &CLD, AM_IMPL },      { &CMP, AM_ABS_Y },   { NULL, AM_UK },    { NULL, AM_UK }, { NULL, AM_UK },   { &CMP, AM_ABS_X },  { &DEC, AM_ABS_X },   { NULL, AM_UK },
/* 0xEX  */    { &CPX, AM_IMM },   { &SBC, AM_X_IND }, { NULL, AM_UK },  { NULL, AM_UK }, { &CPX, AM_ZPG },      { &SBC, AM_ZPG },    { &INC, AM_ZPG },   { NULL, AM_UK }, { &INX, AM_IMPL },      { &SBC, AM_IMM },     { &NOP, AM_IMPL },  { NULL, AM_UK }, { &CPX, AM_ABS },  { &SBC, AM_ABS },    { &INC, AM_ABS },     { NULL, AM_UK },
/* 0xFX  */    { &BEQ, AM_REL },   { &SBC, AM_IND_Y }, { NULL, AM_UK },  { NULL, AM_UK }, { NULL, AM_UK },       { &SBC, AM_ZPG_X },  { &INC, AM_ZPG_X }, { NULL, AM_UK }, { &SED, AM_IMPL },      { &SBC, AM_ABS_Y },   { NULL, AM_UK },    { NULL, AM_UK }, { NULL, AM_UK },   { &SBC, AM_ABS_X },  { &INC, AM_ABS_X },   { NULL, AM_UK }
};
