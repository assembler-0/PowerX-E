#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MEMORY_SIZE (256 * 1024 * 1024)  // 256MB default
#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE - 1)
#define PAGE_SHIFT 12

// TLB Entry for fast address translation
typedef struct {
    uint64_t vaddr;     // Virtual address tag
    uint64_t paddr;     // Physical address
    uint32_t flags;     // Protection flags
    bool valid;
} tlb_entry_t;

#define TLB_SIZE 64
#define TLB_MASK (TLB_SIZE - 1)

// Memory subsystem
typedef struct {
    uint8_t* ram;
    size_t ram_size;
    
    // Simple TLB for address translation
    tlb_entry_t itlb[TLB_SIZE];  // Instruction TLB
    tlb_entry_t dtlb[TLB_SIZE];  // Data TLB
    
    // Stats for optimization
    uint64_t tlb_hits;
    uint64_t tlb_misses;
} memory_system_t;

// Memory access flags
#define MEM_READ    (1 << 0)
#define MEM_WRITE   (1 << 1)
#define MEM_EXEC    (1 << 2)

// Fast inline functions for common operations
static inline uint64_t vaddr_to_tlb_index(uint64_t vaddr) {
    return (vaddr >> PAGE_SHIFT) & TLB_MASK;
}

// Function prototypes
bool memory_init(memory_system_t* mem, size_t size);
void memory_destroy(memory_system_t* mem);

// Fast memory access functions
uint8_t memory_read8(memory_system_t* mem, uint64_t addr);
uint16_t memory_read16(memory_system_t* mem, uint64_t addr);
uint32_t memory_read32(memory_system_t* mem, uint64_t addr);
uint64_t memory_read64(memory_system_t* mem, uint64_t addr);

void memory_write8(memory_system_t* mem, uint64_t addr, uint8_t value);
void memory_write16(memory_system_t* mem, uint64_t addr, uint16_t value);
void memory_write32(memory_system_t* mem, uint64_t addr, uint32_t value);
void memory_write64(memory_system_t* mem, uint64_t addr, uint64_t value);

// TLB management
void tlb_flush(memory_system_t* mem);
bool tlb_lookup(tlb_entry_t* tlb, uint64_t vaddr, uint64_t* paddr);
void tlb_insert(tlb_entry_t* tlb, uint64_t vaddr, uint64_t paddr, uint32_t flags);

#endif