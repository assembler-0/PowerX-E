#include "emulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool emulator_init(powerpc_emulator_t* emu, const emulator_config_t* config) {
    memset(emu, 0, sizeof(powerpc_emulator_t));
    
    if (config) {
        emu->config = *config;
    } else {
        emu->config = emulator_default_config();
    }
    
    cpu_reset(&emu->cpu);
    
    if (!memory_init(&emu->memory, emu->config.memory_size)) {
        return false;
    }
    
    emu->running = false;
    emu->halt_requested = false;
    
    return true;
}

void emulator_destroy(powerpc_emulator_t* emu) {
    memory_destroy(&emu->memory);
    memset(emu, 0, sizeof(powerpc_emulator_t));
}

void emulator_reset(powerpc_emulator_t* emu) {
    cpu_reset(&emu->cpu);
    tlb_flush(&emu->memory);
    emu->instructions_executed = 0;
    emu->cycles = 0;
    emu->branches_taken = 0;
    emu->branches_not_taken = 0;
    emu->running = false;
    emu->halt_requested = false;
}

bool emulator_load_binary(powerpc_emulator_t* emu, const char* filename, uint64_t load_addr) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (load_addr + size > emu->memory.ram_size) {
        printf("Error: Binary too large for memory\n");
        fclose(file);
        return false;
    }
    
    size_t read = fread(&emu->memory.ram[load_addr], 1, size, file);
    fclose(file);
    
    if (read != size) {
        printf("Error: Failed to read entire file\n");
        return false;
    }
    
    emu->cpu.pc = load_addr;
    printf("Loaded %ld bytes at 0x%lx\n", size, load_addr);
    return true;
}

static void execute_instruction(powerpc_emulator_t* emu, const ppc_instruction_t* inst) {
    // Basic instruction execution - implement key instructions
    switch (inst->opcode) {
        case PPC_OP_ADDI:
            if (inst->ra == 0) {
                emu->cpu.gpr[inst->rt] = inst->simm;
            } else {
                emu->cpu.gpr[inst->rt] = emu->cpu.gpr[inst->ra] + inst->simm;
            }
            break;
            
        case PPC_OP_ADDIS:
            if (inst->ra == 0) {
                emu->cpu.gpr[inst->rt] = (int64_t)inst->simm << 16;
            } else {
                emu->cpu.gpr[inst->rt] = emu->cpu.gpr[inst->ra] + ((int64_t)inst->simm << 16);
            }
            break;
            
        case PPC_OP_ORI:
            emu->cpu.gpr[inst->rt] = emu->cpu.gpr[inst->ra] | inst->imm;
            break;
            
        case PPC_OP_LWZ:
            {
                uint64_t addr = (inst->ra == 0) ? 0 : emu->cpu.gpr[inst->ra];
                addr += inst->simm;
                emu->cpu.gpr[inst->rt] = memory_read32(&emu->memory, addr);
            }
            break;
            
        case PPC_OP_LBZ:
            {
                uint64_t addr = (inst->ra == 0) ? 0 : emu->cpu.gpr[inst->ra];
                addr += inst->simm;
                emu->cpu.gpr[inst->rt] = memory_read8(&emu->memory, addr);
            }
            break;
            
        case PPC_OP_STW:
            {
                uint64_t addr = (inst->ra == 0) ? 0 : emu->cpu.gpr[inst->ra];
                addr += inst->simm;
                memory_write32(&emu->memory, addr, emu->cpu.gpr[inst->rt]);
            }
            break;
            
        case PPC_OP_B:
            if (inst->aa) {
                emu->cpu.pc = inst->addr;
            } else {
                emu->cpu.pc += inst->addr;
            }
            if (inst->lk) {
                emu->cpu.lr = emu->cpu.pc + 4;
            }
            emu->cpu.pc -= 4; // Compensate for PC increment
            break;
            
        case PPC_OP_BC: // Conditional Branch
            {
                uint8_t bo = (inst->raw >> 21) & 0x1F;
                uint8_t bi = (inst->raw >> 16) & 0x1F;
                bool ctr_ok = true, cond_ok = true;
                
                // Check CTR condition
                if (!(bo & 0x04)) {
                    emu->cpu.ctr--;
                    ctr_ok = ((emu->cpu.ctr != 0) ^ ((bo & 0x02) != 0));
                }
                
                // Check CR condition
                if (!(bo & 0x10)) {
                    uint32_t cr_bit = (emu->cpu.cr >> (31 - bi)) & 1;
                    cond_ok = (cr_bit == ((bo & 0x08) >> 3));
                }
                
                if (ctr_ok && cond_ok) {
                    if (inst->aa) {
                        emu->cpu.pc = inst->addr;
                    } else {
                        emu->cpu.pc += inst->addr;
                    }
                    if (inst->lk) {
                        emu->cpu.lr = emu->cpu.pc + 4;
                    }
                    emu->cpu.pc -= 4;
                    emu->branches_taken++;
                } else {
                    emu->branches_not_taken++;
                }
            }
            break;
            
        case PPC_OP_CMPI:
            {
                int64_t a = (int64_t)emu->cpu.gpr[inst->ra];
                int64_t b = (int64_t)inst->simm;
                uint8_t crfield = (inst->raw >> 23) & 0x7;
                uint32_t result = 0;
                
                if (a < b) result |= 8;      // LT
                else if (a > b) result |= 4; // GT  
                else result |= 2;            // EQ
                
                if (emu->cpu.xer & XER_SO) result |= 1; // SO
                
                emu->cpu.cr = (emu->cpu.cr & ~(0xF << (28 - crfield * 4))) | 
                             (result << (28 - crfield * 4));
            }
            break;
            
        case PPC_OP_X_FORM:
            // Handle X-form instructions
            switch (inst->extended_op) {
                case 266: // add
                    emu->cpu.gpr[inst->rt] = emu->cpu.gpr[inst->ra] + emu->cpu.gpr[inst->rb];
                    break;
                case 40: // subf
                    emu->cpu.gpr[inst->rt] = emu->cpu.gpr[inst->rb] - emu->cpu.gpr[inst->ra];
                    break;
                case 28: // and
                    emu->cpu.gpr[inst->rt] = emu->cpu.gpr[inst->ra] & emu->cpu.gpr[inst->rb];
                    break;
                case 444: // or
                    emu->cpu.gpr[inst->rt] = emu->cpu.gpr[inst->ra] | emu->cpu.gpr[inst->rb];
                    break;
                case 316: // xor
                    emu->cpu.gpr[inst->rt] = emu->cpu.gpr[inst->ra] ^ emu->cpu.gpr[inst->rb];
                    break;
                case 235: // mullw - Multiply Low Word (extended opcode 235)
                    {
                        int64_t result = (int64_t)(int32_t)emu->cpu.gpr[inst->ra] * 
                                        (int64_t)(int32_t)emu->cpu.gpr[inst->rb];
                        emu->cpu.gpr[inst->rt] = (uint32_t)result;
                        if (inst->oe) {
                            // Set overflow if result doesn't fit in 32 bits
                            if (result != (int32_t)result) {
                                emu->cpu.xer |= XER_OV | XER_SO;
                            }
                        }
                    }
                    break;
            }
            break;
            
        default:
            if (emu->config.enable_trace) {
                printf("Unimplemented instruction: %s (0x%08x)\n", 
                       get_instruction_name(inst), inst->raw);
            }
            break;
    }
}

void emulator_step(powerpc_emulator_t* emu) {
    if (emu->halt_requested) return;
    
    // Fetch instruction
    uint32_t raw_inst = memory_read32(&emu->memory, emu->cpu.pc);
    ppc_instruction_t inst = decode_instruction(raw_inst);
    
    if (emu->config.enable_trace) {
        printf("0x%016lx: %08x  %s\n", emu->cpu.pc, raw_inst, get_instruction_name(&inst));
    }
    
    // Execute instruction
    emu->cpu.pc += 4;
    execute_instruction(emu, &inst);
    
    emu->instructions_executed++;
    emu->cycles++;
    
    // Check for halt conditions
    if (emu->instructions_executed >= emu->config.max_instructions) {
        emu->halt_requested = true;
    }
}

void emulator_run(powerpc_emulator_t* emu) {
    emu->running = true;
    emu->halt_requested = false;
    
    while (emu->running && !emu->halt_requested) {
        emulator_step(emu);
    }
    
    emu->running = false;
}

void emulator_halt(powerpc_emulator_t* emu) {
    emu->halt_requested = true;
    emu->running = false;
}

void emulator_dump_state(const powerpc_emulator_t* emu) {
    cpu_dump_state(&emu->cpu);
    printf("\n=== Execution Stats ===\n");
    printf("Instructions: %lu\n", emu->instructions_executed);
    printf("Cycles: %lu\n", emu->cycles);
    printf("TLB Hits: %lu, Misses: %lu\n", 
           emu->memory.tlb_hits, emu->memory.tlb_misses);
}

void emulator_print_stats(const powerpc_emulator_t* emu) {
    printf("=== Performance Statistics ===\n");
    printf("Instructions executed: %lu\n", emu->instructions_executed);
    printf("Total cycles: %lu\n", emu->cycles);
    printf("IPC: %.2f\n", (double)emu->instructions_executed / emu->cycles);
    printf("TLB hit rate: %.2f%%\n", 
           100.0 * emu->memory.tlb_hits / (emu->memory.tlb_hits + emu->memory.tlb_misses));
    printf("Branches taken: %lu\n", emu->branches_taken);
    printf("Branches not taken: %lu\n", emu->branches_not_taken);
}