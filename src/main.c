#include "emulator.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc __attribute__((unused)), char* argv[] __attribute__((unused))) {
    printf("PowerX-E - PowerPC Emulator\n");
    printf("==========================\n\n");
    
    powerpc_emulator_t emu;
    emulator_config_t config = emulator_default_config();
    config.enable_trace = true;
    config.max_instructions = 1000;
    
    if (!emulator_init(&emu, &config)) {
        printf("Failed to initialize emulator\n");
        return 1;
    }
    
    // Extended test program with new instructions
    uint32_t test_program[] = {
        0x38600005,  // addi r3, r0, 5     # r3 = 5
        0x38800003,  // addi r4, r0, 3     # r4 = 3  
        0x7CA421D6,  // mullw r5, r4, r3   # r5 = 4 * 3 = 15 (XO=235)
        0x2C050010,  // cmpi cr0, r5, 16   # compare r5 with 16
        0x38C00042,  // addi r6, r0, 66    # r6 = 66 (0x42)
        0x48000000   // b 0 (infinite loop)
    };
    
    // Load test program into memory
    for (int i = 0; i < 4; i++) {
        memory_write32(&emu.memory, 0x100 + i * 4, test_program[i]);
    }
    
    printf("Running test program...\n\n");
    
    // Execute a few instructions
    for (int i = 0; i < 5; i++) {
        emulator_step(&emu);
    }
    
    printf("\nFinal state:\n");
    emulator_dump_state(&emu);
    
    printf("\nExpected: r3=5, r4=3, r5=15, r6=66\n");
    printf("Actual:   r3=%lu, r4=%lu, r5=%lu, r6=%lu\n", 
           emu.cpu.gpr[3], emu.cpu.gpr[4], emu.cpu.gpr[5], emu.cpu.gpr[6]);
    printf("CR0 field: %s%s%s%s\n",
           CR_LT(emu.cpu.cr, 0) ? "LT " : "",
           CR_GT(emu.cpu.cr, 0) ? "GT " : "", 
           CR_EQ(emu.cpu.cr, 0) ? "EQ " : "",
           CR_SO(emu.cpu.cr, 0) ? "SO" : "");
    
    if (emu.cpu.gpr[3] == 5 && emu.cpu.gpr[4] == 3 && emu.cpu.gpr[5] == 15 && emu.cpu.gpr[6] == 66) {
        printf("\n✓ Test PASSED! Extended instructions work correctly.\n");
    } else {
        printf("\n✗ Test FAILED! Check instruction implementation.\n");
    }
    
    emulator_destroy(&emu);
    return 0;
}