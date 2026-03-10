#ifndef BHARAT_MM_H
#define BHARAT_MM_H

#include <stdint.h>

/*
 * Bharat-OS Memory Management Subsystem
 * Supports NUMA-awareness for datacenter scalability and standard paging.
 */

// Basic page definitions (Assuming 4KB base pages)
#define PAGE_SIZE 4096

typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;

#include "list.h"
#include "mm_coloring.h"

// Page metadata structure for Buddy Allocator
typedef struct page {
  list_head_t list;
  uint16_t ref_count;
  uint16_t numa_node;
  uint32_t flags;
  // For buddy system: an order field or similar can be kept, or inferred.
  // For simplicity, we can keep the order in the list if needed,
  // but typically a page's block order is stored in metadata.
  int order;
} page_t;

// Convert page struct pointer back to physical address
phys_addr_t page_to_phys(page_t *page);
page_t *phys_to_page(phys_addr_t phys);

// NUMA Node Definition
typedef struct {
  uint32_t node_id;
  phys_addr_t start_addr;
  uint64_t total_pages;
  uint64_t free_pages;
  // Implementation specific bitmap or buddy allocator metadata here
  void *allocator_metadata;
} numa_node_t;

// Virtual Memory Page Flags
#define PAGE_COW 0x100  // Copy-on-Write Flag
#define PAGE_USER 0x200 // User accessible flag

// Buddy Allocator page flags
#define PAGE_FLAG_RESERVED (1 << 0)
#define PAGE_FLAG_KERNEL (1 << 1)
#define PAGE_FLAG_USER (1 << 2)

// Initialize physical memory allocator natively using Multiboot/SBI memory maps
int mm_pmm_init(uint32_t magic, void *memory_map);

// Base Page Allocation (NUMA aware)
phys_addr_t mm_alloc_page(uint32_t preferred_numa_node);
phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node,
                                 uint32_t flags);
phys_addr_t pmm_alloc_pages_colored(int order, uint32_t preferred_numa_node,
                                    uint32_t flags,
                                    mm_color_config_t *color_config);
void mm_free_page(phys_addr_t page);

typedef enum {
    BHARAT_DMA_COHERENT      = 1u << 0,
    BHARAT_DMA_UNCACHED      = 1u << 1,
    BHARAT_DMA_WRITE_COMBINE = 1u << 2,
    BHARAT_DMA_32BIT_ONLY    = 1u << 3,
    BHARAT_DMA_ZERO          = 1u << 4,
} bharat_dma_flags_t;

int mm_alloc_dma_pages(size_t size,
                       uint32_t preferred_numa_node,
                       uint32_t dma_flags,
                       phys_addr_t *out_phys,
                       void **out_kernel_virt);
int mm_free_dma_pages(phys_addr_t phys, void *kernel_virt, size_t size);

// Support for Copy-on-Write (CoW) page reference counting
void mm_inc_page_ref(phys_addr_t page);

// Virtual Memory Management (Architecture agnostic paging)
typedef struct {
  phys_addr_t root_table; // CR3 on x86, satp on RISC-V
} address_space_t;

int vmm_init(void);
int vmm_map_page(virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags);
int vmm_unmap_page(virt_addr_t vaddr);

int mm_vmm_map_page(address_space_t *as, virt_addr_t vaddr, phys_addr_t paddr,
                    uint32_t flags);
int mm_vmm_unmap_page(address_space_t *as, virt_addr_t vaddr);

// Forward declaration for capability_token_t is not straightforward because
// it's a typedef of an anonymous struct in formal_verif.h. So we include it
// directly.
#include "advanced/formal_verif.h"
int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t *cap,
                        int is_npu);

#include "mm/address_token.h"
int vmm_map_device_mmio_token(virt_addr_t vaddr, phys_addr_t paddr,
                              uint64_t size, const bharat_addr_token_t *token,
                              int is_npu);

// Create a new empty hardware address space
address_space_t *mm_create_address_space(void);

#endif // BHARAT_MM_H

void tlb_shootdown(virt_addr_t vaddr);
