#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <bharat/cap/cap_validate.h>
#include "../core/services/namesvc/src/registry.h"
#include <bharat/cap/cap.h>

bool bharat_cap_is_valid(bharat_cap_handle_t handle) {
    return handle != 0;
}

void test_registry_init_and_empty() {
    namesvc_registry_init();
    bharat_cap_handle_t ep;
    int32_t res = namesvc_registry_lookup("nonexistent", 0, false, &ep, NULL, NULL, NULL);
    assert(res == NAMESVC_STATUS_ERR_NOTFOUND);
}

void test_registry_add_and_lookup() {
    namesvc_registry_init();
    bharat_cap_handle_t ep1 = 100;
    bharat_cap_handle_t ep2 = 200;

    int32_t res = namesvc_registry_add("service_a", 0, 1, 0, ep1);
    assert(res == NAMESVC_STATUS_OK);

    res = namesvc_registry_add("service_b", 0, 1, 0, ep2);
    assert(res == NAMESVC_STATUS_OK);

    bharat_cap_handle_t out_ep;
    res = namesvc_registry_lookup("service_a", 0, false, &out_ep, NULL, NULL, NULL);
    assert(res == NAMESVC_STATUS_OK);
    assert(out_ep == ep1);

    res = namesvc_registry_lookup("service_b", 0, false, &out_ep, NULL, NULL, NULL);
    assert(res == NAMESVC_STATUS_OK);
    assert(out_ep == ep2);
}

void test_registry_add_invalid() {
    namesvc_registry_init();
    int32_t res = namesvc_registry_add("", 0, 0, 0, 100);
    assert(res == NAMESVC_STATUS_ERR_INVAL);

    res = namesvc_registry_add("bad_cap", 0, 0, 0, 0);
    assert(res == NAMESVC_STATUS_ERR_INVAL);
}

void test_registry_duplicate() {
    namesvc_registry_init();
    bharat_cap_handle_t ep1 = 100;
    int32_t res = namesvc_registry_add("service_a", 0, 1, 0, ep1);
    assert(res == NAMESVC_STATUS_OK);

    res = namesvc_registry_add("service_a", 0, 1, 0, ep1);
    assert(res == NAMESVC_STATUS_ERR_EXISTS);
}

void test_registry_remove() {
    namesvc_registry_init();
    bharat_cap_handle_t ep1 = 100;

    int32_t res = namesvc_registry_add("service_a", 0, 0, 0, ep1);
    assert(res == NAMESVC_STATUS_OK);

    res = namesvc_registry_remove("service_a", ep1);
    assert(res == NAMESVC_STATUS_OK);

    bharat_cap_handle_t out_ep;
    bharat_service_id_t sid;
    uint32_t ver, flags;
    res = namesvc_registry_lookup("service_a", 0, false, &out_ep, &sid, &ver, &flags);
    assert(res == NAMESVC_STATUS_ERR_NOTFOUND);

    res = namesvc_registry_remove("service_a", ep1);
    assert(res == NAMESVC_STATUS_ERR_NOTFOUND);
}

void test_registry_exhaustion() {
    namesvc_registry_init();

    char namebuf[32];
    for (int i = 0; i < 256; i++) {
        sprintf(namebuf, "svc%d", i);
        int32_t res = namesvc_registry_add(namebuf, 0, 0, 0, i + 1);
        assert(res == NAMESVC_STATUS_OK);
    }

    int32_t res = namesvc_registry_add("toomany", 0, 0, 0, 500);
    assert(res == NAMESVC_STATUS_ERR_NOSPACE);

    res = namesvc_registry_remove("svc0", 1);
    assert(res == NAMESVC_STATUS_OK);

    res = namesvc_registry_add("nowfits", 0, 0, 0, 500);
    assert(res == NAMESVC_STATUS_OK);
}

void test_registry_long_name() {
    namesvc_registry_init();

    char long_name[256];
    memset(long_name, 'A', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';

    // Now we validate length correctly and reject overlong names
    int32_t res = namesvc_registry_add(long_name, 0, 0, 0, 100);
    assert(res == NAMESVC_STATUS_ERR_INVAL);
}

void test_registry_unauthorized_remove() {
    namesvc_registry_init();
    int32_t res = namesvc_registry_add("service_a", 0, 0, 0, 100);
    assert(res == NAMESVC_STATUS_OK);

    res = namesvc_registry_remove("service_a", 200); // Try removing with wrong cap
    assert(res == NAMESVC_STATUS_ERR_NOTFOUND); // Or permission denied
}

void test_registry_stale_replace() {
    namesvc_registry_init();
    g_force_stale = false;
    int32_t res = namesvc_registry_add("service_stale", 0, 1, 0, 999);
    assert(res == NAMESVC_STATUS_OK);

    g_force_stale = true;
    // Try replacing stale endpoint with a new one. Handle 888 means it's the SAME owner as before (999).
    res = namesvc_registry_add("service_stale", 0, 1, 0, 888);
    assert(res == NAMESVC_STATUS_OK);

    // Now try replacing with DIFFERENT owner (100)
    // First we make 888 stale so we reach the owner check
    // Wait, let's register a new stale service for different owner test
    g_force_stale = false;
    res = namesvc_registry_add("service_stale2", 0, 1, 0, 999);
    assert(res == NAMESVC_STATUS_OK);

    g_force_stale = true; // 999 is now stale
    res = namesvc_registry_add("service_stale2", 0, 1, 0, 100); // 100 is different owner than 999
    assert(res == NAMESVC_STATUS_ERR_EXISTS);
}

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
        if (handle == 888 || handle == 999) {
            out_result->descriptor.owner_principal_id = 999;
        } else {
            out_result->descriptor.owner_principal_id = handle;
        }
    }
    return BHARAT_CAP_OK;
}

int main() {
    printf("Running namesvc registry tests...\n");
    test_registry_init_and_empty();
    test_registry_add_and_lookup();
    test_registry_add_invalid();
    test_registry_duplicate();
    test_registry_remove();
    test_registry_exhaustion();
    test_registry_long_name();
    test_registry_unauthorized_remove();
    test_registry_stale_replace();
    printf("All namesvc registry tests passed.\n");
    return 0;
}
