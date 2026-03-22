#include "mm/prot_domain.h"
#include "slab.h"
#include <stddef.h>

#define ERR_NOT_SUPPORTED -1

static prot_domain_t* prot_none_create(void) {
    prot_domain_t* domain = (prot_domain_t*)kmalloc(sizeof(prot_domain_t));
    if (!domain) return NULL;

    domain->mode = PROT_MODE_NONE;
    domain->backend_state = NULL;
    return domain;
}

static void prot_none_destroy(prot_domain_t* domain) {
    if (!domain) return;
    kfree(domain);
}

static void prot_none_activate(prot_domain_t* domain) {
    (void)domain;
    // No-op for identity
}

static int prot_none_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    (void)domain; (void)vaddr; (void)paddr; (void)size; (void)flags;
    return ERR_NOT_SUPPORTED;
}

static int prot_none_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    (void)domain; (void)vaddr; (void)size;
    return ERR_NOT_SUPPORTED;
}

static int prot_none_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    (void)domain; (void)vaddr; (void)size; (void)flags;
    return ERR_NOT_SUPPORTED;
}

static int prot_none_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags) {
    (void)domain;
    // In flat/none mode, identity mapping is assumed.
    if (paddr) *paddr = vaddr;
    if (flags) *flags = 0; // Default flat flags
    return 0;
}

prot_domain_ops_t prot_none_ops = {
    .create = prot_none_create,
    .destroy = prot_none_destroy,
    .activate = prot_none_activate,
    .map_region = prot_none_map_region,
    .unmap_region = prot_none_unmap_region,
    .protect_region = prot_none_protect_region,
    .query_region = prot_none_query_region,
};
