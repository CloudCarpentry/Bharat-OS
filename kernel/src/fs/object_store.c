#include "fs/object_store.h"
#include "kernel/status.h"

// PHASE1_COMPAT_SHIM: Object store policy moved out of kernel.

int object_store_register(object_store_t* store, capability_t* cap) {
    (void)store;
    (void)cap;
    return K_ERR_REQUIRES_FS_SERVICE;
}

int object_store_lookup(const char* uri, object_store_t** out_store, capability_t* cap) {
    (void)uri;
    (void)cap;
    if (out_store) *out_store = NULL;
    return K_ERR_REQUIRES_FS_SERVICE;
}

#ifdef TESTING
void object_store_test_reset_state(void) {
}
#endif
