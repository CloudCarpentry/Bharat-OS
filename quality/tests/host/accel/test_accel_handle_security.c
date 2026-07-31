#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "accelmgr_broker.h"

// Testing basic opaque handle layout and lookup
static void test_handle_layout(void) {
    uint64_t handle = bh_handle_make(12, 1005);
    assert(bh_handle_index(handle) == 12);
    assert(bh_handle_generation(handle) == 1005);
}

static void test_object_registration_lookup(void) {
    init_accelmgr();

    // Normal lookup of pre-registered devices
    int err = 0;
    broker_object_t *npu = lookup_object(g_npu_handle, BH_ACCEL_OBJECT_DEVICE, &err);
    assert(npu != NULL);
    assert(err == 0);
    assert(npu->u.device.device_id == 0);

    // Wrong object type lookup
    broker_object_t *wrong = lookup_object(g_npu_handle, BH_ACCEL_OBJECT_QUEUE, &err);
    assert(wrong == NULL);
    assert(err == -104);

    // Invalid index
    wrong = lookup_object(bh_handle_make(0, 1), BH_ACCEL_OBJECT_DEVICE, &err);
    assert(wrong == NULL);
    assert(err == -101);

    // Unregistered / Stale handle
    wrong = lookup_object(bh_handle_make(15, 1), BH_ACCEL_OBJECT_DEVICE, &err);
    assert(wrong == NULL);
    assert(err == -102);
}

int main(void) {
    printf("Running test_accel_handle_security...\n");
    test_handle_layout();
    test_object_registration_lookup();
    printf("test_accel_handle_security PASSED\n");
    return 0;
}
