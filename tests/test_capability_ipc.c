#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/capability.h"
#include "../kernel/include/ipc_endpoint.h"

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

void entry_point(void) {}

int main(void) {
    sched_init();

    kprocess_t* proc = process_create("ipc");
    assert(proc != NULL);

    capability_table_t* table = (capability_table_t*)proc->security_sandbox_ctx;
    assert(table != NULL);

    uint32_t send_cap = 0;
    uint32_t recv_cap = 0;
    assert(ipc_endpoint_create(table, &send_cap, &recv_cap) == IPC_OK);

    const char* msg = "hello";
    assert(ipc_endpoint_send(table, send_cap, msg, 5U) == IPC_OK);

    char out[16] = {0};
    uint32_t out_len = 0;
    assert(ipc_endpoint_receive(table, recv_cap, out, sizeof(out), &out_len) == IPC_OK);
    assert(out_len == 5U);
    assert(out[0] == 'h' && out[4] == 'o');

    // Delegation must only narrow rights.
    uint32_t delegated = 0;
    assert(cap_table_delegate(table, table, send_cap, CAP_PERM_SEND, &delegated) == 0);

    printf("Capability/endpoint IPC tests passed.\n");
    return 0;
}
