#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bh_personality_registry.h"
#include "bh_personality.h"
#include "bh_process_personality.h"

// Mock ops
static int mock_val = 42;

void test_personality_registry(void) {
    printf("Testing personality registry...\n");

    // Initial state
    assert(bh_personality_registry_get_ops(BH_PERSONALITY_NATIVE) == NULL);
    assert(bh_personality_registry_get_ops(BH_PERSONALITY_LINUX) == NULL);

    // Register
    bh_personality_registry_register(BH_PERSONALITY_NATIVE, &mock_val);
    assert(bh_personality_registry_get_ops(BH_PERSONALITY_NATIVE) == &mock_val);
    assert(bh_personality_registry_get_ops(BH_PERSONALITY_LINUX) == NULL);

    bh_personality_registry_register(BH_PERSONALITY_LINUX, &mock_val);
    assert(bh_personality_registry_get_ops(BH_PERSONALITY_LINUX) == &mock_val);

    printf("Personality registry tests passed.\n");
}

void test_process_personality(void) {
    printf("Testing process personality structures...\n");
    bh_process_personality_t p = {
        .kind = BH_PERSONALITY_LINUX,
        .error_domain = BH_ERROR_DOMAIN_LINUX,
        .handle_space = BH_HANDLE_SPACE_POSIX_FD,
        .abi_flags = 0x1234
    };
    assert(p.kind == BH_PERSONALITY_LINUX);
    assert(p.error_domain == BH_ERROR_DOMAIN_LINUX);
    assert(p.handle_space == BH_HANDLE_SPACE_POSIX_FD);
    assert(p.abi_flags == 0x1234);
    printf("Process personality tests passed.\n");
}

int main(void) {
    test_personality_registry();
    test_process_personality();
    return 0;
}
