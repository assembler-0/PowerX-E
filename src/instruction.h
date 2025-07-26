#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>
#include <stdbool.h>

// PowerPC instruction formats
typedef enum {
    PPC_FMT_I,      // I-form (branch)
    PPC_FMT_B,      // B-form (conditional branch)  
    PPC_FMT_SC,     // SC-form (system call)
    PPC_FMT_D,      // D-form (load/store, arithmetic with immediate)
    PPC_FMT_DS,     // DS-form (64-bit load/store)
    PPC_FMT_X,      // X-form (register-register operations)
    PPC_FMT_XL,     // XL-form (condition register logical)
    PPC_FMT_XFX,    // XFX-form (move to/from special registers)
    PPC_FMT_XFL,    // XFL-form (floating point)
    PPC_FMT_A,      // A-form (floating point)
    PPC_FMT_M,      // M-form (rotate and mask)
    PPC_FMT_MD,     // MD-form (64-bit rotate and mask)
    PPC_FMT_MDS,    // MDS-form (64-bit rotate and mask)
    PPC_FMT_UNKNOWN
} ppc_inst_format_t;

// Decoded instruction structure
typedef struct {
    uint32_t raw;           // Raw 32-bit instruction
    ppc_inst_format_t fmt;  // Instruction format
    uint16_t opcode;        // Primary opcode
    uint16_t extended_op;   // Extended opcode for X-form, etc.
    
    // Operand fields (not all used for every instruction)
    uint8_t rt, ra, rb;     // Register operands
    uint8_t bt, ba, bb;     // Bit field operands
    uint8_t frt, fra, frb, frc; // Floating point registers
    uint16_t imm;           // Immediate value
    int16_t simm;           // Sign-extended immediate
    uint32_t addr;          // Address for branches
    uint16_t spr;           // Special purpose register
    uint8_t sh, mb, me;     // Shift and mask fields
    
    // Flags
    bool rc;                // Record bit
    bool oe;                // Overflow enable
    bool lk;                // Link bit
    bool aa;                // Absolute address
} ppc_instruction_t;

// Instruction decode table entry
typedef struct {
    uint32_t mask;
    uint32_t match;
    ppc_inst_format_t format;
    const char* mnemonic;
} decode_entry_t;

// Fast instruction decoding
ppc_instruction_t decode_instruction(uint32_t raw_inst);
const char* get_instruction_name(const ppc_instruction_t* inst);

// Instruction field extraction macros
#define INST_OPCODE(x)      (((x) >> 26) & 0x3F)
#define INST_RT(x)          (((x) >> 21) & 0x1F)
#define INST_RA(x)          (((x) >> 16) & 0x1F)
#define INST_RB(x)          (((x) >> 11) & 0x1F)
#define INST_RC(x)          ((x) & 1)
#define INST_SIMM(x)        ((int16_t)((x) & 0xFFFF))
#define INST_UIMM(x)        ((x) & 0xFFFF)
#define INST_LI(x)          (((int32_t)((x) & 0x3FFFFFC)) << 6 >> 6)
#define INST_BD(x)          (((int16_t)((x) & 0xFFFC)))
#define INST_XO_X(x)        (((x) >> 1) & 0x3FF)
#define INST_XO_XL(x)       (((x) >> 1) & 0x3FF)

// Additional field extraction macros
#define INST_BT(x)          (((x) >> 21) & 0x1F)
#define INST_BA(x)          (((x) >> 16) & 0x1F)
#define INST_BB(x)          (((x) >> 11) & 0x1F)
#define INST_SH(x)          (((x) >> 11) & 0x1F)
#define INST_MB(x)          (((x) >> 6) & 0x1F)
#define INST_ME(x)          (((x) >> 1) & 0x1F)
#define INST_SPR(x)         ((((x) >> 16) & 0x1F) | (((x) >> 6) & 0x3E0))
#define INST_FRT(x)         (((x) >> 21) & 0x1F)
#define INST_FRA(x)         (((x) >> 16) & 0x1F)
#define INST_FRB(x)         (((x) >> 11) & 0x1F)
#define INST_FRC(x)         (((x) >> 6) & 0x1F)
#define INST_OE(x)          (((x) >> 10) & 1)
#define INST_AA(x)          (((x) >> 1) & 1)
#define INST_LK(x)          ((x) & 1)

// Common PowerPC opcodes
#define PPC_OP_TWI          3
#define PPC_OP_MULLI        7
#define PPC_OP_SUBFIC       8
#define PPC_OP_CMPLI        10
#define PPC_OP_CMPI         11
#define PPC_OP_ADDIC        12
#define PPC_OP_ADDIC_DOT    13
#define PPC_OP_ADDI         14
#define PPC_OP_ADDIS        15
#define PPC_OP_BC           16
#define PPC_OP_SC           17
#define PPC_OP_B            18
#define PPC_OP_MCRF         19
#define PPC_OP_BCLR         19
#define PPC_OP_RFID         19
#define PPC_OP_RLWIMI       20
#define PPC_OP_RLWINM       21
#define PPC_OP_RLWNM        23
#define PPC_OP_ORI          24
#define PPC_OP_ORIS         25
#define PPC_OP_XORI         26
#define PPC_OP_XORIS        27
#define PPC_OP_ANDI_DOT     28
#define PPC_OP_ANDIS_DOT    29
#define PPC_OP_RLDIC        30
#define PPC_OP_X_FORM       31
#define PPC_OP_LWZ          32
#define PPC_OP_LWZU         33
#define PPC_OP_LBZ          34
#define PPC_OP_LBZU         35
#define PPC_OP_STW          36
#define PPC_OP_STWU         37
#define PPC_OP_STB          38
#define PPC_OP_STBU         39
#define PPC_OP_LHZ          40
#define PPC_OP_LHZU         41
#define PPC_OP_LHA          42
#define PPC_OP_LHAU         43
#define PPC_OP_STH          44
#define PPC_OP_STHU         45
#define PPC_OP_LMW          46
#define PPC_OP_STMW         47

#endif