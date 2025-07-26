#include "instruction.h"
#include <stdio.h>

int main() {
    uint32_t test_instructions[] = {
        0x38600005,  // addi r3, r0, 5
        0x7C601A14,  // add r3, r0, r3
        0x48000000   // b 0
    };
    
    for (int i = 0; i < 3; i++) {
        ppc_instruction_t inst = decode_instruction(test_instructions[i]);
        printf("0x%08X: %s (fmt=%d)\n", 
               inst.raw, 
               get_instruction_name(&inst), 
               inst.fmt);
    }
    
    return 0;
}