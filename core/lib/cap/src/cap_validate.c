#include <bharat/cap/cap_validate.h>
#include <stddef.h>

static bharat_cap_validate_fn_t g_test_backend = NULL;

void bharat_cap_set_validate_backend_for_tests(bharat_cap_validate_fn_t fn) {
    g_test_backend = fn;
}

bharat_cap_status_t bharat_cap_validate(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result)
{
    if (g_test_backend) {
        return g_test_backend(handle, expected_object_type, expected_object_id, required_rights, required_scope, out_result);
    }

    // Default production path (not fully implemented in Phase 1, waiting on CapMgr/Syscall runtime mapping)
    // In a complete system this would issue an IPC to the capability service or a syscall.
    // For now, if no test backend is set, we fail closed.

    if (out_result) {
        out_result->allowed = false;
        out_result->status = BHARAT_CAP_UNSUPPORTED;
    }

    return BHARAT_CAP_UNSUPPORTED;
}
