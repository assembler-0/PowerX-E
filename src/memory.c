#include "memory.h"
#include <stdlib.h>
#include <string.h>

bool memory_init(memory_system_t* mem, size_t size) {
    mem->ram = aligned_alloc(4096, size);
    if (!mem->ram) return false;
    
    mem->ram_size = size;
    memset(mem->ram, 0, size);
    
    // Initialize TLBs
    memset(mem->itlb, 0, sizeof(mem->itlb));
    memset(mem->dtlb, 0, sizeof(mem->dtlb));
    
    // Set up identity mapping for initial pages
    for (int i = 0; i < 16; i++) {
        uint64_t addr = i * PAGE_SIZE;
        tlb_insert(mem->itlb, addr, addr, MEM_READ | MEM_EXEC);
        tlb_insert(mem->dtlb, addr, addr, MEM_READ | MEM_WRITE);
    }
    
    mem->tlb_hits = 0;
    mem->tlb_misses = 0;
    return true;
}

void memory_destroy(memory_system_t* mem) {
    if (mem->ram) {
        free(mem->ram);
        mem->ram = NULL;
    }
}

bool tlb_lookup(tlb_entry_t* tlb, uint64_t vaddr, uint64_t* paddr) {
    uint64_t index = vaddr_to_tlb_index(vaddr);
    tlb_entry_t* entry = &tlb[index];
    
    if (entry->valid && (entry->vaddr & ~PAGE_MASK) == (vaddr & ~PAGE_MASK)) {
        *paddr = entry->paddr | (vaddr & PAGE_MASK);
        return true;
    }
    return false;
}

void tlb_insert(tlb_entry_t* tlb, uint64_t vaddr, uint64_t paddr, uint32_t flags) {
    uint64_t index = vaddr_to_tlb_index(vaddr);
    tlb_entry_t* entry = &tlb[index];
    
    entry->vaddr = vaddr & ~PAGE_MASK;
    entry->paddr = paddr & ~PAGE_MASK;
    entry->flags = flags;
    entry->valid = true;
}

void tlb_flush(memory_system_t* mem) {
    for (int i = 0; i < TLB_SIZE; i++) {
        mem->itlb[i].valid = false;
        mem->dtlb[i].valid = false;
    }
}

static inline uint64_t translate_address(memory_system_t* mem, uint64_t vaddr, bool is_instruction) {
    uint64_t paddr;
    tlb_entry_t* tlb = is_instruction ? mem->itlb : mem->dtlb;
    
    if (tlb_lookup(tlb, vaddr, &paddr)) {
        mem->tlb_hits++;
        return paddr;
    }
    
    mem->tlb_misses++;
    // Simple identity mapping fallback
    paddr = vaddr;
    tlb_insert(tlb, vaddr, paddr, MEM_READ | MEM_WRITE | MEM_EXEC);
    return paddr;
}

uint8_t memory_read8(memory_system_t* mem, uint64_t addr) {
    uint64_t paddr = translate_address(mem, addr, false);
    if (paddr >= mem->ram_size) return 0;
    return mem->ram[paddr];
}

uint16_t memory_read16(memory_system_t* mem, uint64_t addr) {
    uint64_t paddr = translate_address(mem, addr, false);
    if (paddr + 1 >= mem->ram_size) return 0;
    return __builtin_bswap16(*(uint16_t*)&mem->ram[paddr]);
}

uint32_t memory_read32(memory_system_t* mem, uint64_t addr) {
    uint64_t paddr = translate_address(mem, addr, false);
    if (paddr + 3 >= mem->ram_size) return 0;
    return __builtin_bswap32(*(uint32_t*)&mem->ram[paddr]);
}

uint64_t memory_read64(memory_system_t* mem, uint64_t addr) {
    uint64_t paddr = translate_address(mem, addr, false);
    if (paddr + 7 >= mem->ram_size) return 0;
    return __builtin_bswap64(*(uint64_t*)&mem->ram[paddr]);
}

void memory_write8(memory_system_t* mem, uint64_t addr, uint8_t value) {
    uint64_t paddr = translate_address(mem, addr, false);
    if (paddr < mem->ram_size) {
        mem->ram[paddr] = value;
    }
}

void memory_write16(memory_system_t* mem, uint64_t addr, uint16_t value) {
    uint64_t paddr = translate_address(mem, addr, false);
    if (paddr + 1 < mem->ram_size) {
        *(uint16_t*)&mem->ram[paddr] = __builtin_bswap16(value);
    }
}

void memory_write32(memory_system_t* mem, uint64_t addr, uint32_t value) {
    uint64_t paddr = translate_address(mem, addr, false);
    if (paddr + 3 < mem->ram_size) {
        *(uint32_t*)&mem->ram[paddr] = __builtin_bswap32(value);
    }
}

void memory_write64(memory_system_t* mem, uint64_t addr, uint64_t value) {
    uint64_t paddr = translate_address(mem, addr, false);
    if (paddr + 7 < mem->ram_size) {
        *(uint64_t*)&mem->ram[paddr] = __builtin_bswap64(value);
    }
}