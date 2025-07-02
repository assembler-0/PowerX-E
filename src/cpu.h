#ifndef CPU_STATE_H
#define CPU_STATE_H

#include <stdint.h>
#include <stdbool.h>

// PowerPC Register File - optimized for cache alignment
typedef struct {
    // General Purpose Registers (32 x 64-bit for 64-bit PowerPC)
    uint64_t gpr[32];
    
    // Floating Point Registers (32 x 64-bit)
    double fpr[32];

    // Special Purpose Registers (most commonly used first)
    uint64_t pc;        // Program Counter
    uint64_t lr;        // Link Register  
    uint64_t ctr;       // Count Register
    uint32_t cr;        // Condition Register
    uint32_t xer;       // Fixed-Point Exception Register
    uint64_t msr;       // Machine State Register
    uint32_t vrsave;
    double fpscr;
    // Memory Management
    uint64_t dar;       // Data Address Register
    uint32_t dsisr;     // Data Storage Interrupt Status Register
    
    // Cache-aligned execution state
    struct {
        bool reservation_valid;
        uint64_t reservation_addr;
        uint32_t fpscr;     // Floating Point Status Control Register
        uint32_t _padding;
    } exec_state;
    
} __attribute__((aligned(64))) ppc_cpu_state_t;

// Condition Register field extraction macros
#define CR_FIELD(cr, field) (((cr) >> (28 - (field) * 4)) & 0xF)
#define CR_LT(cr, field) (CR_FIELD(cr, field) & 8)
#define CR_GT(cr, field) (CR_FIELD(cr, field) & 4)
#define CR_EQ(cr, field) (CR_FIELD(cr, field) & 2)
#define CR_SO(cr, field) (CR_FIELD(cr, field) & 1)

// XER register bits
#define XER_SO  (1U << 31)  // Summary Overflow
#define XER_OV  (1U << 30)  // Overflow
#define XER_CA  (1U << 29)  // Carry

// MSR register bits (key ones)
#define MSR_SF  (1ULL << 63) // 64-bit mode
#define MSR_EE  (1ULL << 48) // External Interrupt Enable
#define MSR_PR  (1ULL << 49) // Problem State
#define MSR_IR  (1ULL << 58) // Instruction Relocate
#define MSR_DR  (1ULL << 59) // Data Relocate

void cpu_reset(ppc_cpu_state_t* cpu);
void cpu_dump_state(const ppc_cpu_state_t* cpu);

#endif