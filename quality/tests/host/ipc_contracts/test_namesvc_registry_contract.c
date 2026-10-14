#include <bharat/uapi/ipc/contract.h>
#include <bharat/uapi/ipc/status.h>
#include <bharat/uapi/namesvc/contract.h>
#include <ipc_dispatch.h>
#include <registry.h>
#include <service_manifest.h>
#include <bharat/cap/cap_validate.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

bool bharat_cap_is_valid(bharat_cap_handle_t cap) { return cap != BHARAT_CAP_INVALID_HANDLE; }

// Simple mock for capability validation
bharat_cap_status_t bharat_cap_validate(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result)
{
    (void)expected_object_type;
    (void)expected_object_id;
    (void)required_rights;
    (void)required_scope;

    if (out_result) {
        out_result->allowed = true;
        out_result->status = BHARAT_CAP_OK;
        out_result->descriptor.owner_principal_id = handle;
    }
    return BHARAT_CAP_OK;
}

void test_namesvc_registry() {
    namesvc_registry_init();

    // Register
    int result = namesvc_registry_add("com.bharat.net", 0, 1, 0, 10);
    assert(result == NAMESVC_STATUS_OK);

    // Duplicate incompatible registration (same exact version)
    result = namesvc_registry_add("com.bharat.net", 0, 1, 0, 11);
    assert(result == NAMESVC_STATUS_ERR_EXISTS);

    // Register newer version
    result = namesvc_registry_add("com.bharat.net", 0, 2, 0, 12);
    assert(result == NAMESVC_STATUS_OK);

    bharat_cap_handle_t endpoint;
    uint32_t version;
    uint32_t flags;

    // Lookup exact version 1
    result = namesvc_registry_lookup("com.bharat.net", 1, true, &endpoint, NULL, &version, &flags);
    assert(result == NAMESVC_STATUS_OK);
    assert(endpoint == 10);

    // Lookup compatible version (should get highest >= 1)
    result = namesvc_registry_lookup("com.bharat.net", 1, false, &endpoint, NULL, &version, &flags);
    assert(result == NAMESVC_STATUS_OK);
    assert(endpoint == 12); // Got v2

    // Remove version 1
    result = namesvc_registry_remove("com.bharat.net", 10);
    assert(result == NAMESVC_STATUS_OK);

    // Exact lookup v1 should now fail
    result = namesvc_registry_lookup("com.bharat.net", 1, true, &endpoint, NULL, &version, &flags);
    assert(result == NAMESVC_STATUS_ERR_NOTFOUND);

    printf("test_namesvc_registry passed\n");
}

void test_namesvc_manifest() {
    assert(strcmp(namesvc_service_manifest.service_name, "namesvc") == 0);
    assert(namesvc_service_manifest.interface_version == 1);
    assert(namesvc_service_manifest.operation_count > 0);
    printf("test_namesvc_manifest passed\n");
}

int main() {
    test_namesvc_registry();
    test_namesvc_manifest();
    return 0;
}
