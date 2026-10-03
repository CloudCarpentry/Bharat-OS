#include <stdio.h>
#include <assert.h>
#include "compat/android/android_security.h"

int main() {
    printf("Running test_android_security...\n");

    android_sec_sid_t process_sid = 1;
    android_logical_obj_t obj = {0};
    int prot = 0;

    assert(android_sec_check_memory_access(process_sid, &obj, prot) == 0);

    android_sec_sid_t current_sid = 1;
    android_sec_sid_t new_sid = 2;

    assert(android_sec_transition_domain(current_sid, new_sid) == 0);

    printf("test_android_security passed!\n");
    return 0;
}
