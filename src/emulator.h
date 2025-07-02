#ifndef EMULATOR_H
#define EMULATOR_H

#include "cpu_state.h"
#include "memory.h"
#include "instruction.h"
#include <stdint.h>
#include <stdbool.h>

// Emulator configuration
typedef struct {
    bool enable_trace;
    bool enable_stats;
    uint64_t max_instructions;
    uint32_t memory_size;
} emulator_config_t;

// Main emulator state
typedef struct {
    ppc_cpu_state_t cpu;
    memory_system_t memory;
    emulator_config_t config;
    
    // Execution statistics
    uint64_t instructions_executed;
    uint64_t cycles;
    uint64_t branches_taken;
    uint64_t branches_not_taken;
    
    // Control flags
    bool running;
    bool halt_requested;
} powerpc_emulator_t;

// Emulator lifecycle
bool emulator_init(powerpc_emulator_t* emu, const emulator_config_t* config);
void emulator_destroy(powerpc_emulator_t* emu);
void emulator_reset(powerpc_emulator_t* emu);

// Execution control
bool emulator_load_binary(powerpc_emulator_t* emu, const char* filename, uint64_t load_addr);
void emulator_run(powerpc_emulator_t* emu);
void emulator_step(powerpc_emulator_t* emu);
void emulator_halt(powerpc_emulator_t* emu);

// Debugging and introspection
void emulator_dump_state(const powerpc_emulator_t* emu);
void emulator_print_stats(const powerpc_emulator_t* emu);

// Default configuration
static inline emulator_config_t emulator_default_config(void) {
    return (emulator_config_t) {
        .enable_trace = false,
        .enable_stats = true,
        .max_instructions = UINT64_MAX,
        .memory_size = MEMORY_SIZE
    };
}

#endif