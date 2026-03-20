#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../services/namesvc/src/registry.h"
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

int main() {
    printf("Running namesvc registry tests...\n");
    test_registry_init_and_empty();
    test_registry_add_and_lookup();
    test_registry_add_invalid();
    test_registry_duplicate();
    test_registry_remove();
    printf("All namesvc registry tests passed.\n");
    return 0;
}
