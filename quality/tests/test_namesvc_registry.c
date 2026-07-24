#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../core/services/namesvc/src/registry.h"
#include <bharat/cap/cap.h>

bool bharat_cap_is_valid(bharat_cap_handle_t handle) {
    return handle != 0;
}

void test_registry_init_and_empty() {
    namesvc_registry_init();
    bharat_cap_handle_t ep;
    int32_t res = namesvc_registry_lookup("nonexistent", &ep);
    assert(res == NAMESVC_STATUS_ERR_NOTFOUND);
}

void test_registry_add_and_lookup() {
    namesvc_registry_init();
    bharat_cap_handle_t ep1 = 100;
    bharat_cap_handle_t ep2 = 200;

    int32_t res = namesvc_registry_add("service_a", ep1);
    assert(res == NAMESVC_STATUS_OK);

    res = namesvc_registry_add("service_b", ep2);
    assert(res == NAMESVC_STATUS_OK);

    bharat_cap_handle_t out_ep;
    res = namesvc_registry_lookup("service_a", &out_ep);
    assert(res == NAMESVC_STATUS_OK);
    assert(out_ep == ep1);

    res = namesvc_registry_lookup("service_b", &out_ep);
    assert(res == NAMESVC_STATUS_OK);
    assert(out_ep == ep2);
}

void test_registry_add_invalid() {
    namesvc_registry_init();
    int32_t res = namesvc_registry_add("", 100);
    assert(res == NAMESVC_STATUS_ERR_INVAL);

    res = namesvc_registry_add("bad_cap", 0);
    assert(res == NAMESVC_STATUS_ERR_INVAL);
}

void test_registry_duplicate() {
    namesvc_registry_init();
    bharat_cap_handle_t ep1 = 100;
    int32_t res = namesvc_registry_add("service_a", ep1);
    assert(res == NAMESVC_STATUS_OK);

    res = namesvc_registry_add("service_a", ep1);
    assert(res == NAMESVC_STATUS_ERR_EXISTS);
}

void test_registry_remove() {
    namesvc_registry_init();
    bharat_cap_handle_t ep1 = 100;

    int32_t res = namesvc_registry_add("service_a", ep1);
    assert(res == NAMESVC_STATUS_OK);

    res = namesvc_registry_remove("service_a");
    assert(res == NAMESVC_STATUS_OK);

    bharat_cap_handle_t out_ep;
    res = namesvc_registry_lookup("service_a", &out_ep);
    assert(res == NAMESVC_STATUS_ERR_NOTFOUND);

    res = namesvc_registry_remove("service_a");
    assert(res == NAMESVC_STATUS_ERR_NOTFOUND);
}

void test_registry_exhaustion() {
    namesvc_registry_init();

    char namebuf[32];
    for (int i = 0; i < 256; i++) {
        sprintf(namebuf, "svc%d", i);
        int32_t res = namesvc_registry_add(namebuf, i + 1);
        assert(res == NAMESVC_STATUS_OK);
    }

    int32_t res = namesvc_registry_add("toomany", 999);
    assert(res == NAMESVC_STATUS_ERR_NOSPACE);

    res = namesvc_registry_remove("svc0");
    assert(res == NAMESVC_STATUS_OK);

    res = namesvc_registry_add("nowfits", 999);
    assert(res == NAMESVC_STATUS_OK);
}

void test_registry_long_name() {
    namesvc_registry_init();

    char long_name[256];
    memset(long_name, 'A', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';

    // The service doesn't validate name length natively yet, it truncates.
    // However, it will truncate to NAMESVC_MAX_NAME_LEN. Let's make sure it handles
    // lookup correctly when truncated.
    int32_t res = namesvc_registry_add(long_name, 100);
    assert(res == NAMESVC_STATUS_OK);

    // Test that the long name we just added can be found when querying with the same long string.
    bharat_cap_handle_t out_ep;
    res = namesvc_registry_lookup(long_name, &out_ep);
    assert(res == NAMESVC_STATUS_OK);
    assert(out_ep == 100);

    // Provide a query string that matches exactly the truncated version
    char truncated[NAMESVC_MAX_NAME_LEN];
    strncpy(truncated, long_name, NAMESVC_MAX_NAME_LEN - 1);
    truncated[NAMESVC_MAX_NAME_LEN - 1] = '\0';
    res = namesvc_registry_lookup(truncated, &out_ep);
    assert(res == NAMESVC_STATUS_OK);
    assert(out_ep == 100);

    // Test removing it using the original long string
    res = namesvc_registry_remove(long_name);
    assert(res == NAMESVC_STATUS_OK);
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
    printf("All namesvc registry tests passed.\n");
    return 0;
}
