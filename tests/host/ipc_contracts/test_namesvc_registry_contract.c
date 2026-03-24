#include <bharat/uapi/ipc/contract.h>
#include <bharat/uapi/ipc/status.h>
#include <bharat/namesvc/namesvc_ipc.h>
#include <ipc_dispatch.h>
#include <registry.h>
#include <service_manifest.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

bool bharat_cap_is_valid(bharat_cap_handle_t cap) { return cap != BHARAT_CAP_INVALID_HANDLE; }

void test_namesvc_registry() {
    namesvc_registry_init();

    // Register
    int result = namesvc_registry_add("com.bharat.net", "Iface1", 1, 0, 10);
    assert(result == NAMESVC_STATUS_OK);

    // Duplicate incompatible registration (same exact version)
    result = namesvc_registry_add("com.bharat.net", "Iface1", 1, 0, 11);
    assert(result == NAMESVC_STATUS_ERR_EXISTS);

    // Register newer version
    result = namesvc_registry_add("com.bharat.net", "Iface1", 2, 0, 12);
    assert(result == NAMESVC_STATUS_OK);

    bharat_cap_handle_t endpoint;
    uint32_t version;
    uint32_t flags;

    // Lookup exact version 1
    result = namesvc_registry_lookup("com.bharat.net", "Iface1", 1, true, &endpoint, &version, &flags);
    assert(result == NAMESVC_STATUS_OK);
    assert(endpoint == 10);

    // Lookup compatible version (should get highest >= 1)
    result = namesvc_registry_lookup("com.bharat.net", "Iface1", 1, false, &endpoint, &version, &flags);
    assert(result == NAMESVC_STATUS_OK);
    assert(endpoint == 12); // Got v2

    // Remove version 1
    result = namesvc_registry_remove("com.bharat.net", "Iface1", 1);
    assert(result == NAMESVC_STATUS_OK);

    // Exact lookup v1 should now fail
    result = namesvc_registry_lookup("com.bharat.net", "Iface1", 1, true, &endpoint, &version, &flags);
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