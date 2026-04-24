#ifndef BHARAT_MM_PROT_DOMAIN_H
#define BHARAT_MM_PROT_DOMAIN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PROT_MODE_NONE,
    PROT_MODE_MMU_FULL,
    PROT_MODE_MMU_LITE,
    PROT_MODE_MPU_ONLY,
    PROT_MODE_MIXED
} prot_mode_t;

struct prot_domain;
typedef struct prot_domain prot_domain_t;

typedef struct prot_domain_ops {
    prot_domain_t* (*create)(void);
    void (*destroy)(prot_domain_t* domain);
    void (*activate)(prot_domain_t* domain);
    int (*map_region)(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags);
    int (*unmap_region)(prot_domain_t* domain, uintptr_t vaddr, size_t size);
    int (*protect_region)(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags);
    int (*query_region)(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags);
} prot_domain_ops_t;

struct prot_domain {
    prot_mode_t mode;
    const prot_domain_ops_t* ops;
    void* backend_state;
};

// Global Initialization
void prot_domain_init(void);

// Core API
prot_domain_t* prot_domain_create(void);
void prot_domain_destroy(prot_domain_t* domain);
void prot_domain_activate(prot_domain_t* domain);
int prot_domain_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags);
int prot_domain_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size);
int prot_domain_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags);
int prot_domain_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags);

// Return the actively selected prototype domain backend (used for boot verification)
prot_domain_ops_t* prot_domain_get_active_backend(void);

#endif // BHARAT_MM_PROT_DOMAIN_H
