#include "arch/arch_caps.h"

// Define a test-wide mock capability structure that tests can optionally manipulate
arch_caps_t g_test_mock_arch_caps = {
    .bits = 0xFFFFFFFF
};

arch_caps_t arch_get_caps(void) {
    return g_test_mock_arch_caps;
}
