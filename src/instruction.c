#include "instruction.h"
#include <string.h>

// Fast decode table for primary opcodes
static const decode_entry_t primary_decode_table[] = {
    // D-form instructions
    {0xFC000000, 0x0C000000, PPC_FMT_D,  "twi"},
    {0xFC000000, 0x1C000000, PPC_FMT_D,  "mulli"},
    {0xFC000000, 0x20000000, PPC_FMT_D,  "subfic"},
    {0xFC000000, 0x28000000, PPC_FMT_D,  "cmpli"},
    {0xFC000000, 0x2C000000, PPC_FMT_D,  "cmpi"},
    {0xFC000000, 0x30000000, PPC_FMT_D,  "addic"},
    {0xFC000000, 0x34000000, PPC_FMT_D,  "addic."},
    {0xFC000000, 0x38000000, PPC_FMT_D,  "addi"},
    {0xFC000000, 0x3C000000, PPC_FMT_D,  "addis"},
    
    // Branch instructions
    {0xFC000000, 0x40000000, PPC_FMT_B,  "bc"},
    {0xFC000000, 0x44000000, PPC_FMT_SC, "sc"},
    {0xFC000000, 0x48000000, PPC_FMT_I,  "b"},
    
    // M-form rotate instructions
    {0xFC000000, 0x50000000, PPC_FMT_M,  "rlwimi"},
    {0xFC000000, 0x54000000, PPC_FMT_M,  "rlwinm"},
    {0xFC000000, 0x5C000000, PPC_FMT_M,  "rlwnm"},
    
    // More D-form
    {0xFC000000, 0x60000000, PPC_FMT_D,  "ori"},
    {0xFC000000, 0x64000000, PPC_FMT_D,  "oris"},
    {0xFC000000, 0x68000000, PPC_FMT_D,  "xori"},
    {0xFC000000, 0x6C000000, PPC_FMT_D,  "xoris"},
    {0xFC000000, 0x70000000, PPC_FMT_D,  "andi."},
    {0xFC000000, 0x74000000, PPC_FMT_D,  "andis."},
    
    // Load/Store instructions
    {0xFC000000, 0x80000000, PPC_FMT_D,  "lwz"},
    {0xFC000000, 0x84000000, PPC_FMT_D,  "lwzu"},
    {0xFC000000, 0x88000000, PPC_FMT_D,  "lbz"},
    {0xFC000000, 0x8C000000, PPC_FMT_D,  "lbzu"},
    {0xFC000000, 0x90000000, PPC_FMT_D,  "stw"},
    {0xFC000000, 0x94000000, PPC_FMT_D,  "stwu"},
    {0xFC000000, 0x98000000, PPC_FMT_D,  "stb"},
    {0xFC000000, 0x9C000000, PPC_FMT_D,  "stbu"},
    {0xFC000000, 0xA0000000, PPC_FMT_D,  "lhz"},
    {0xFC000000, 0xA4000000, PPC_FMT_D,  "lhzu"},
    {0xFC000000, 0xA8000000, PPC_FMT_D,  "lha"},
    {0xFC000000, 0xAC000000, PPC_FMT_D,  "lhau"},
    {0xFC000000, 0xB0000000, PPC_FMT_D,  "sth"},
    {0xFC000000, 0xB4000000, PPC_FMT_D,  "sthu"},
    {0xFC000000, 0xB8000000, PPC_FMT_D,  "lmw"},
    {0xFC000000, 0xBC000000, PPC_FMT_D,  "stmw"},
    
    // Floating point
    {0xFC000000, 0xC0000000, PPC_FMT_D,  "lfs"},
    {0xFC000000, 0xC4000000, PPC_FMT_D,  "lfsu"},
    {0xFC000000, 0xC8000000, PPC_FMT_D,  "lfd"},
    {0xFC000000, 0xCC000000, PPC_FMT_D,  "lfdu"},
    {0xFC000000, 0xD0000000, PPC_FMT_D,  "stfs"},
    {0xFC000000, 0xD4000000, PPC_FMT_D,  "stfsu"},
    {0xFC000000, 0xD8000000, PPC_FMT_D,  "stfd"},
    {0xFC000000, 0xDC000000, PPC_FMT_D,  "stfdu"},
    
    {0, 0, PPC_FMT_UNKNOWN, NULL}
};

// X-form extended opcodes (opcode 31)
static const decode_entry_t x_form_decode_table[] = {
    // Arithmetic
    {0xFC0007FE, 0x7C000214, PPC_FMT_X, "add"},
    {0xFC0007FE, 0x7C000014, PPC_FMT_X, "addc"},
    {0xFC0007FE, 0x7C000114, PPC_FMT_X, "adde"},
    {0xFC0007FE, 0x7C000194, PPC_FMT_X, "addze"},
    {0xFC0007FE, 0x7C0001D4, PPC_FMT_X, "addme"},
    {0xFC0007FE, 0x7C000050, PPC_FMT_X, "subf"},
    {0xFC0007FE, 0x7C000010, PPC_FMT_X, "subfc"},
    {0xFC0007FE, 0x7C000110, PPC_FMT_X, "subfe"},
    {0xFC0007FE, 0x7C000190, PPC_FMT_X, "subfze"},
    {0xFC0007FE, 0x7C0001D0, PPC_FMT_X, "subfme"},
    {0xFC0007FE, 0x7C000034, PPC_FMT_X, "cntlzw"},
    
    // Logical
    {0xFC0007FE, 0x7C000038, PPC_FMT_X, "and"},
    {0xFC0007FE, 0x7C000078, PPC_FMT_X, "andc"},
    {0xFC0007FE, 0x7C000378, PPC_FMT_X, "or"},
    {0xFC0007FE, 0x7C000338, PPC_FMT_X, "orc"},
    {0xFC0007FE, 0x7C000278, PPC_FMT_X, "xor"},
    {0xFC0007FE, 0x7C0000F8, PPC_FMT_X, "nor"},
    {0xFC0007FE, 0x7C000238, PPC_FMT_X, "eqv"},
    {0xFC0007FE, 0x7C0003B8, PPC_FMT_X, "nand"},
    
    // Shifts
    {0xFC0007FE, 0x7C000030, PPC_FMT_X, "slw"},
    {0xFC0007FE, 0x7C000430, PPC_FMT_X, "srw"},
    {0xFC0007FE, 0x7C000630, PPC_FMT_X, "sraw"},
    {0xFC0007FE, 0x7C000670, PPC_FMT_X, "srawi"},
    
    // Multiply/Divide
    {0xFC0007FE, 0x7C0002D6, PPC_FMT_X, "mulhw"},
    {0xFC0007FE, 0x7C000296, PPC_FMT_X, "mulhwu"},
    {0xFC0007FE, 0x7C0001D6, PPC_FMT_X, "mullw"},
    {0xFC0007FE, 0x7C0003D6, PPC_FMT_X, "divw"},
    {0xFC0007FE, 0x7C000396, PPC_FMT_X, "divwu"},
    
    // Compare
    {0xFC0007FE, 0x7C000000, PPC_FMT_X, "cmp"},
    {0xFC0007FE, 0x7C000040, PPC_FMT_X, "cmpl"},
    
    // Load/Store indexed
    {0xFC0007FE, 0x7C00002E, PPC_FMT_X, "lwzx"},
    {0xFC0007FE, 0x7C00006E, PPC_FMT_X, "lwzux"},
    {0xFC0007FE, 0x7C0000AE, PPC_FMT_X, "lbzx"},
    {0xFC0007FE, 0x7C0000EE, PPC_FMT_X, "lbzux"},
    {0xFC0007FE, 0x7C00012E, PPC_FMT_X, "stwx"},
    {0xFC0007FE, 0x7C00016E, PPC_FMT_X, "stwux"},
    {0xFC0007FE, 0x7C0001AE, PPC_FMT_X, "stbx"},
    {0xFC0007FE, 0x7C0001EE, PPC_FMT_X, "stbux"},
    {0xFC0007FE, 0x7C00022E, PPC_FMT_X, "lhzx"},
    {0xFC0007FE, 0x7C00026E, PPC_FMT_X, "lhzux"},
    {0xFC0007FE, 0x7C0002AE, PPC_FMT_X, "lhax"},
    {0xFC0007FE, 0x7C0002EE, PPC_FMT_X, "lhaux"},
    {0xFC0007FE, 0x7C00032E, PPC_FMT_X, "sthx"},
    {0xFC0007FE, 0x7C00036E, PPC_FMT_X, "sthux"},
    
    // Special
    {0xFC0007FE, 0x7C0004AC, PPC_FMT_X, "sync"},
    {0xFC0007FE, 0x7C0000A6, PPC_FMT_X, "mfmsr"},
    {0xFC0007FE, 0x7C000124, PPC_FMT_X, "mtmsr"},
    
    {0, 0, PPC_FMT_UNKNOWN, NULL}
};

// XL-form opcodes (opcode 19)
static const decode_entry_t xl_form_decode_table[] = {
    {0xFC0007FE, 0x4C000020, PPC_FMT_XL, "bclr"},
    {0xFC0007FE, 0x4C000420, PPC_FMT_XL, "bcctr"},
    {0xFC0007FE, 0x4C000202, PPC_FMT_XL, "crand"},
    {0xFC0007FE, 0x4C000102, PPC_FMT_XL, "crandc"},
    {0xFC0007FE, 0x4C000242, PPC_FMT_XL, "creqv"},
    {0xFC0007FE, 0x4C0001C2, PPC_FMT_XL, "crnand"},
    {0xFC0007FE, 0x4C000042, PPC_FMT_XL, "crnor"},
    {0xFC0007FE, 0x4C000382, PPC_FMT_XL, "cror"},
    {0xFC0007FE, 0x4C000342, PPC_FMT_XL, "crorc"},
    {0xFC0007FE, 0x4C000282, PPC_FMT_XL, "crxor"},
    {0xFC0007FE, 0x4C000000, PPC_FMT_XL, "mcrf"},
    {0, 0, PPC_FMT_UNKNOWN, NULL}
};

// XFX-form opcodes (opcode 31)
static const decode_entry_t xfx_form_decode_table[] = {
    {0xFC0007FE, 0x7C0002A6, PPC_FMT_XFX, "mfspr"},
    {0xFC0007FE, 0x7C0003A6, PPC_FMT_XFX, "mtspr"},
    {0xFC0007FE, 0x7C000026, PPC_FMT_XFX, "mfcr"},
    {0xFC0007FE, 0x7C000120, PPC_FMT_XFX, "mtcrf"},
    {0, 0, PPC_FMT_UNKNOWN, NULL}
};

// A-form floating point opcodes
static const decode_entry_t a_form_decode_table[] = {
    {0xFC00003E, 0xFC00002A, PPC_FMT_A, "fadd"},
    {0xFC00003E, 0xFC000028, PPC_FMT_A, "fsub"},
    {0xFC00003E, 0xFC000032, PPC_FMT_A, "fmul"},
    {0xFC00003E, 0xFC000024, PPC_FMT_A, "fdiv"},
    {0xFC00003E, 0xFC00002E, PPC_FMT_A, "fsel"},
    {0xFC00003E, 0xFC00003A, PPC_FMT_A, "fmadd"},
    {0xFC00003E, 0xFC000038, PPC_FMT_A, "fmsub"},
    {0xFC00003E, 0xFC00003E, PPC_FMT_A, "fnmadd"},
    {0xFC00003E, 0xFC00003C, PPC_FMT_A, "fnmsub"},
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

    switch (inst.opcode) {
        case 0:   break; // Reserved
        case 1:   break; // Reserved
        case 2:   break; // tdi (64-bit)
        case 3:   break; // twi (handled in primary table)
        case 4:   break; // Vector/VMX instructions
        case 5:   break; // Reserved
        case 6:   break; // Reserved
        case 7:   break; // mulli (handled in primary table)
        case 8:   break; // subfic (handled in primary table)
        case 9:   break; // Reserved
        case 10:  break; // cmpli (handled in primary table)
        case 11:  break; // cmpi (handled in primary table)
        case 12:  break; // addic (handled in primary table)
        case 13:  break; // addic. (handled in primary table)
        case 14:  break; // addi (handled in primary table)
        case 15:  break; // addis (handled in primary table)
        case 16:  break; // bc (handled in primary table)
        case 17:  break; // sc (handled in primary table)
        case 18:  break; // b (handled in primary table)
        case 19: {
            inst.extended_op = INST_XO_XL(raw_inst);
            for (const decode_entry_t* entry = xl_form_decode_table; entry->mnemonic; entry++) {
                if ((raw_inst & entry->mask) == entry->match) {
                    inst.fmt = entry->format;
                    break;
                }
            }
        } break;
        case 20:  break; // rlwimi (handled in primary table)
        case 21:  break; // rlwinm (handled in primary table)
        case 22:  break; // Reserved
        case 23:  break; // rlwnm (handled in primary table)
        case 24:  break; // ori (handled in primary table)
        case 25:  break; // oris (handled in primary table)
        case 26:  break; // xori (handled in primary table)
        case 27:  break; // xoris (handled in primary table)
        case 28:  break; // andi. (handled in primary table)
        case 29:  break; // andis. (handled in primary table)
        case 30:  break; // 64-bit rotate instructions
        case 31: {
            inst.extended_op = INST_XO_X(raw_inst);
            for (const decode_entry_t* entry = x_form_decode_table; entry->mnemonic; entry++) {
                if ((raw_inst & entry->mask) == entry->match) {
                    inst.fmt = entry->format;
                    break;
                }
            }
            // Try XFX-form if not found
            if (inst.fmt == PPC_FMT_UNKNOWN) {
                for (const decode_entry_t* entry = xfx_form_decode_table; entry->mnemonic; entry++) {
                    if ((raw_inst & entry->mask) == entry->match) {
                        inst.fmt = entry->format;
                        break;
                    }
                }
            }
        } break;
        case 32:  break; // lwz (handled in primary table)
        case 33:  break; // lwzu (handled in primary table)
        case 34:  break; // lbz (handled in primary table)
        case 35:  break; // lbzu (handled in primary table)
        case 36:  break; // stw (handled in primary table)
        case 37:  break; // stwu (handled in primary table)
        case 38:  break; // stb (handled in primary table)
        case 39:  break; // stbu (handled in primary table)
        case 40:  break; // lhz (handled in primary table)
        case 41:  break; // lhzu (handled in primary table)
        case 42:  break; // lha (handled in primary table)
        case 43:  break; // lhau (handled in primary table)
        case 44:  break; // sth (handled in primary table)
        case 45:  break; // sthu (handled in primary table)
        case 46:  break; // lmw (handled in primary table)
        case 47:  break; // stmw (handled in primary table)
        case 48:  break; // lfs (handled in primary table)
        case 49:  break; // lfsu (handled in primary table)
        case 50:  break; // lfd (handled in primary table)
        case 51:  break; // lfdu (handled in primary table)
        case 52:  break; // stfs (handled in primary table)
        case 53:  break; // stfsu (handled in primary table)
        case 54:  break; // stfd (handled in primary table)
        case 55:  break; // stfdu (handled in primary table)
        case 56:  break; // 64-bit load/store
        case 57:  break; // 64-bit load/store
        case 58:  break; // 64-bit load/store
        case 59: {
            for (const decode_entry_t* entry = a_form_decode_table; entry->mnemonic; entry++) {
                if ((raw_inst & entry->mask) == entry->match) {
                    inst.fmt = entry->format;
                    break;
                }
            }
        } break;
        case 60:  break; // Reserved
        case 61:  break; // Reserved
        case 62:  break; // 64-bit load/store
        case 63: {
            for (const decode_entry_t* entry = a_form_decode_table; entry->mnemonic; entry++) {
                if ((raw_inst & entry->mask) == entry->match) {
                    inst.fmt = entry->format;
                    break;
                }
            }
        } break;
        default:  break; // Unknown opcode
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
            inst.bt = (raw_inst >> 21) & 0x1F;
            inst.ba = (raw_inst >> 16) & 0x1F;
            inst.bb = (raw_inst >> 11) & 0x1F;
            break;
        case PPC_FMT_SC:
            // System call has no additional fields
            break;
        case PPC_FMT_D:
            // Already extracted rt, ra, simm/imm above
            break;
        case PPC_FMT_DS:
            // 64-bit load/store with DS field
            inst.simm = (int16_t)(raw_inst & 0xFFFC);
            break;
        case PPC_FMT_X:
            // Already extracted rt, ra, rb above
            inst.rc = raw_inst & 1;
            inst.oe = (raw_inst >> 10) & 1;
            break;
        case PPC_FMT_XL:
            inst.bt = (raw_inst >> 21) & 0x1F;
            inst.ba = (raw_inst >> 16) & 0x1F;
            inst.bb = (raw_inst >> 11) & 0x1F;
            inst.lk = raw_inst & 1;
            break;
        case PPC_FMT_XFX:
            // SPR field is split: bits 11-20
            inst.spr = ((raw_inst >> 16) & 0x1F) | (((raw_inst >> 11) & 0x1F) << 5);
            break;
        case PPC_FMT_XFL:
            // Floating point status/control
            break;
        case PPC_FMT_A:
            inst.frt = (raw_inst >> 21) & 0x1F;
            inst.fra = (raw_inst >> 16) & 0x1F;
            inst.frb = (raw_inst >> 11) & 0x1F;
            inst.frc = (raw_inst >> 6) & 0x1F;
            inst.rc = raw_inst & 1;
            break;
        case PPC_FMT_M:
            // Rotate and mask fields
            inst.sh = (raw_inst >> 11) & 0x1F;
            inst.mb = (raw_inst >> 6) & 0x1F;
            inst.me = (raw_inst >> 1) & 0x1F;
            inst.rc = raw_inst & 1;
            break;
        case PPC_FMT_MD:
            // 64-bit rotate and mask
            break;
        case PPC_FMT_MDS:
            // 64-bit rotate and mask with variable shift
            break;
        case PPC_FMT_UNKNOWN:
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
    
    // Try extended opcodes
    if (inst->opcode == 31) {
        // Try X-form
        for (const decode_entry_t* entry = x_form_decode_table; entry->mnemonic; entry++) {
            if ((inst->raw & entry->mask) == entry->match) {
                return entry->mnemonic;
            }
        }
        // Try XFX-form
        for (const decode_entry_t* entry = xfx_form_decode_table; entry->mnemonic; entry++) {
            if ((inst->raw & entry->mask) == entry->match) {
                return entry->mnemonic;
            }
        }
    }
    // Try XL-form opcodes
    else if (inst->opcode == 19) {
        for (const decode_entry_t* entry = xl_form_decode_table; entry->mnemonic; entry++) {
            if ((inst->raw & entry->mask) == entry->match) {
                return entry->mnemonic;
            }
        }
    }
    // Try A-form floating point
    else if (inst->opcode == 59 || inst->opcode == 63) {
        for (const decode_entry_t* entry = a_form_decode_table; entry->mnemonic; entry++) {
            if ((inst->raw & entry->mask) == entry->match) {
                return entry->mnemonic;
            }
        }
    }
    
    return "unknown";
}