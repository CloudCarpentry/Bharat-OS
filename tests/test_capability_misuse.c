#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/capability.h"

static address_space_t g_as = { .root_table = 0x1000U };

address_space_t* mm_create_address_space(void) {
    return &g_as;
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    (void)preferred_numa_node;
    return 0;
}

void mm_free_page(phys_addr_t page) {
    (void)page;
}

int main(void) {
    sched_init();
    kprocess_t* p = process_create("cap-misuse");
    assert(p != NULL);

    capability_table_t* t = (capability_table_t*)p->security_sandbox_ctx;
    assert(t != NULL);

    uint32_t cap = 0;

    // Invalid rights for endpoint object should fail.
    assert(cap_table_grant(t, CAP_OBJ_ENDPOINT, 1U, CAP_PERM_MAP, &cap) != 0);

    // Valid grant succeeds.
    assert(cap_table_grant(t, CAP_OBJ_ENDPOINT, 1U, CAP_PERM_SEND | CAP_PERM_DELEGATE, &cap) == 0);

    // Delegating unsupported rights should fail.
    uint32_t delegated = 0;
    assert(cap_table_delegate(t, t, cap, CAP_PERM_MAP, &delegated) != 0);

    printf("Capability misuse tests passed.\n");
    return 0;
}
