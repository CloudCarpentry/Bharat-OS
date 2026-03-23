#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "android/android_personality.h"
#include "personality/personality_types.h"

// Stubs for console/logging if needed by the test runner
void hal_serial_write(char c) { (void)c; }
void kernel_panic(const char* msg) { (void)msg; while(1); }

static void test_android_registration_sanity(void) {
    android_personality_t p;
    memset(&p, 0, sizeof(p));

    assert(0 == android_personality_register(&p));
    assert(p.base.id == PERS_TYPE_ANDROID);
    assert(strcmp(p.base.name, "Android") == 0);
    assert(p.base.init != NULL);

    // Test the init callback
    assert(0 == p.base.init(&p.base));
    assert(p.max_binder_nodes_per_core == 1024);
    assert(p.service_manager_root.id == 0);
    assert(p.service_manager_root.generation == 1);

    printf("[PASS] test_android_registration_sanity\n");
}

static void test_android_obj_lookup_sanity(void) {
    android_logical_obj_t obj;
    memset(&obj, 0, sizeof(obj));

    // Lookup service manager root (ID 0)
    assert(0 == android_obj_lookup(0, &obj));
    assert(obj.id == 0);
    assert(obj.home_core == 0);
    assert(obj.generation == 1);
    assert(obj.backing_cap.table == NULL); // Check struct field instead of comparing struct to 0

    // Lookup non-existent ID
    assert(-1 == android_obj_lookup(999, &obj));

    printf("[PASS] test_android_obj_lookup_sanity\n");
}

int main(void) {
    printf("Running Android Personality Phase 0 tests...\n");

    test_android_registration_sanity();
    test_android_obj_lookup_sanity();

    printf("Android Personality Phase 0 tests passed successfully.\n");
    return 0;
}
