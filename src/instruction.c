#include "instruction.h"
#include <string.h>

// Fast decode table for primary opcodes
static const decode_entry_t primary_decode_table[] = {
    {0xFC000000, 0x08000000, PPC_FMT_D,  "subfic"},
    {0xFC000000, 0x0C000000, PPC_FMT_D,  "addic"},
    {0xFC000000, 0x10000000, PPC_FMT_D,  "addic."},
    {0xFC000000, 0x14000000, PPC_FMT_D,  "addi"},
    {0xFC000000, 0x18000000, PPC_FMT_D,  "addis"},
    {0xFC000000, 0x2C000000, PPC_FMT_D,  "cmpi"},
    {0xFC000000, 0x20000000, PPC_FMT_B,  "bc"},
    {0xFC000000, 0x24000000, PPC_FMT_I,  "b"},
    {0xFC000000, 0x30000000, PPC_FMT_D,  "ori"},
    {0xFC000000, 0x34000000, PPC_FMT_D,  "oris"},
    {0xFC000000, 0x38000000, PPC_FMT_D,  "xori"},
    {0xFC000000, 0x3C000000, PPC_FMT_D,  "xoris"},
    {0xFC000000, 0x40000000, PPC_FMT_D,  "andi."},
    {0xFC000000, 0x44000000, PPC_FMT_D,  "andis."},
    {0xFC000000, 0x50000000, PPC_FMT_D,  "lwz"},
    {0xFC000000, 0x54000000, PPC_FMT_D,  "lwzu"},
    {0xFC000000, 0x58000000, PPC_FMT_D,  "lbz"},
    {0xFC000000, 0x5C000000, PPC_FMT_D,  "lbzu"},
    {0xFC000000, 0x60000000, PPC_FMT_D,  "stw"},
    {0xFC000000, 0x64000000, PPC_FMT_D,  "stwu"},
    {0xFC000000, 0x68000000, PPC_FMT_D,  "stb"},
    {0xFC000000, 0x6C000000, PPC_FMT_D,  "stbu"},
    {0xFC000000, 0x70000000, PPC_FMT_D,  "lhz"},
    {0xFC000000, 0x74000000, PPC_FMT_D,  "lhzu"},
    {0xFC000000, 0x78000000, PPC_FMT_D,  "lha"},
    {0xFC000000, 0x7C000000, PPC_FMT_D,  "lhau"},
    {0xFC000000, 0x80000000, PPC_FMT_D,  "sth"},
    {0xFC000000, 0x84000000, PPC_FMT_D,  "sthu"},
    {0, 0, PPC_FMT_UNKNOWN, NULL}
};

// X-form extended opcodes (opcode 31)
static const decode_entry_t x_form_decode_table[] = {
    {0xFC0007FE, 0x7C000214, PPC_FMT_X, "add"},
    {0xFC0007FE, 0x7C000050, PPC_FMT_X, "subf"},
    {0xFC0007FE, 0x7C000038, PPC_FMT_X, "and"},
    {0xFC0007FE, 0x7C000378, PPC_FMT_X, "or"},
    {0xFC0007FE, 0x7C000278, PPC_FMT_X, "xor"},
    {0xFC0007FE, 0x7C0000F8, PPC_FMT_X, "nor"},
    {0xFC0007FE, 0x7C000000, PPC_FMT_X, "cmp"},
    {0xFC0007FE, 0x7C000040, PPC_FMT_X, "cmpl"},
    {0xFC0007FE, 0x7C0001BC, PPC_FMT_X, "sraw"},
    {0xFC0007FE, 0x7C000030, PPC_FMT_X, "slw"},
    {0xFC0007FE, 0x7C000430, PPC_FMT_X, "srw"},
    {0xFC0007FE, 0x7C0002D6, PPC_FMT_X, "mulhw"},
    {0xFC0007FE, 0x7C0001D6, PPC_FMT_X, "mullw"},
    {0xFC0007FE, 0x7C0003D6, PPC_FMT_X, "divw"},
    {0xFC0007FE, 0x7C000396, PPC_FMT_X, "divwu"},
    {0, 0, PPC_FMT_UNKNOWN, NULL}
};

ppc_instruction_t decode_instruction(uint32_t raw_inst) {
    ppc_instruction_t inst = {0};
    inst.raw = raw_inst;
    inst.opcode = INST_OPCODE(raw_inst);
    
    // Try primary opcode table first
    for (const decode_entry_t* entry = primary_decode_table; entry->mnemonic; entry++) {
        if ((raw_inst & entry->mask) == entry->match) {
            inst.fmt = entry->format;
            break;
        }
    }
    
    // Handle X-form instructions (opcode 31)
    if (inst.opcode == 31) {
        inst.extended_op = INST_XO_X(raw_inst);
        for (const decode_entry_t* entry = x_form_decode_table; entry->mnemonic; entry++) {
            if ((raw_inst & entry->mask) == entry->match) {
                inst.fmt = entry->format;
                break;
            }
        }
    }
    
    // Extract common fields based on format
    inst.rt = INST_RT(raw_inst);
    inst.ra = INST_RA(raw_inst);
    inst.rb = INST_RB(raw_inst);
    inst.rc = INST_RC(raw_inst);
    inst.simm = INST_SIMM(raw_inst);
    inst.imm = INST_UIMM(raw_inst);
    
    // Format-specific field extraction
    switch (inst.fmt) {
        case PPC_FMT_I:
            inst.addr = INST_LI(raw_inst);
            inst.lk = raw_inst & 1;
            inst.aa = (raw_inst >> 1) & 1;
            break;
        case PPC_FMT_B:
            inst.addr = INST_BD(raw_inst);
            inst.lk = raw_inst & 1;
            inst.aa = (raw_inst >> 1) & 1;
            break;
        default:
            break;
    }
    
    return inst;
}

const char* get_instruction_name(const ppc_instruction_t* inst) {
    // Try primary opcodes first
    for (const decode_entry_t* entry = primary_decode_table; entry->mnemonic; entry++) {
        if ((inst->raw & entry->mask) == entry->match) {
            return entry->mnemonic;
        }
    }
    
    // Try X-form opcodes
    if (inst->opcode == 31) {
        for (const decode_entry_t* entry = x_form_decode_table; entry->mnemonic; entry++) {
            if ((inst->raw & entry->mask) == entry->match) {
                return entry->mnemonic;
            }
        }
    }
    
    return "unknown";
}