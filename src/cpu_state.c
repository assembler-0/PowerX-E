#include "cpu_state.h"
#include <string.h>
#include <stdio.h>

void cpu_reset(ppc_cpu_state_t* cpu) {
    memset(cpu, 0, sizeof(ppc_cpu_state_t));
    
    // Set initial state for PowerPC
    cpu->msr = MSR_SF;  // Start in 64-bit mode
    cpu->pc = 0x100;    // Typical PowerPC reset vector
    cpu->vrsave = 0;    // No vector registers saved initially
}

void cpu_dump_state(const ppc_cpu_state_t* cpu) {
    printf("=== PowerPC CPU State ===\n");
    printf("PC: 0x%016lx  LR: 0x%016lx  CTR: 0x%016lx\n", 
           cpu->pc, cpu->lr, cpu->ctr);
    printf("MSR: 0x%016lx  CR: 0x%08x  XER: 0x%08x\n", 
           cpu->msr, cpu->cr, cpu->xer);
    
    printf("\nGeneral Purpose Registers:\n");
    for (int i = 0; i < 32; i += 4) {
        printf("r%-2d: %016lx  r%-2d: %016lx  r%-2d: %016lx  r%-2d: %016lx\n",
               i, cpu->gpr[i], i+1, cpu->gpr[i+1], 
               i+2, cpu->gpr[i+2], i+3, cpu->gpr[i+3]);
    }
    
    printf("\nCondition Register Fields:\n");
    for (int i = 0; i < 8; i++) {
        uint32_t field = CR_FIELD(cpu->cr, i);
        printf("CR%d: %c%c%c%c ", i,
               (field & 8) ? 'L' : '-',
               (field & 4) ? 'G' : '-', 
               (field & 2) ? 'E' : '-',
               (field & 1) ? 'S' : '-');
    }
    printf("\n");
}