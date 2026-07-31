#include <assert.h>
#include <stdio.h>
#include "ipc/mk_proto.h"
#include "fake_hal.h"

static kstatus_t mock_handler(
    bh_mk_endpoint_handle_t source_endpoint,
    const bh_mk_wire_message_t *message,
    void *ctx)
{
    (void)source_endpoint; (void)message; (void)ctx;
    return K_OK;
}

int main(void) {
    printf("Running test_mk_endpoints...\n");

    kstatus_t st = bh_mk_fabric_init(1);
    assert(st == K_OK);

    fake_hal_set_cpu_id(0);
    bh_mk_core_fabric_t *f = bh_mk_get_core_fabric(0);
    assert(f);

    // 1. Bind endpoint
    bh_mk_endpoint_handle_t handle;
    bh_mk_endpoint_config_t config = {
        .handler_fn = mock_handler,
        .ctx = (void*)42,
        .message_class = 5,
        .opcode = 10
    };

    st = bh_mk_endpoint_bind(&config, &handle);
    assert(st == K_OK);

    // Verify handle contents
    uint32_t handle_core, handle_core_gen, handle_ep_gen, handle_slot;
    st = bh_mk_handle_unpack(handle, &handle_core, &handle_core_gen, &handle_ep_gen, &handle_slot);
    assert(st == K_OK);
    assert(handle_core == 0);
    assert(handle_slot == 0);
    assert(handle_ep_gen == f->endpoints.entries[0].generation);

    // 2. Resolve endpoint
    bh_mk_endpoint_entry_t *entry = NULL;
    st = bh_mk_endpoint_resolve(f, handle, &entry);
    assert(st == K_OK);
    assert(entry->handler_fn == mock_handler);
    assert(entry->ctx == (void*)42);

    // 3. Unbind endpoint
    st = bh_mk_endpoint_unbind(handle);
    assert(st == K_OK);

    // Resolving again should return STALE
    st = bh_mk_endpoint_resolve(f, handle, &entry);
    assert(st == K_ERR_CAP_STALE);

    // 4. Fill endpoints table
    for (int i = 0; i < BH_MK_MAX_ENDPOINTS; i++) {
        bh_mk_endpoint_handle_t temp_handle;
        st = bh_mk_endpoint_bind(&config, &temp_handle);
        assert(st == K_OK);
    }

    // Next bind should fail
    bh_mk_endpoint_handle_t fail_handle;
    st = bh_mk_endpoint_bind(&config, &fail_handle);
    assert(st == K_ERR_NO_RESOURCES);

    printf("test_mk_endpoints PASSED\n");
    return 0;
}
