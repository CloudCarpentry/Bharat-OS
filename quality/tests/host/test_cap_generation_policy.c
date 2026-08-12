#include <assert.h>
#include <stdint.h>

#include "capability.h"

int main(void) {
    const uint32_t current_generation = 7U;

    assert(bh_cap_generation_matches(current_generation, current_generation));
    assert(!bh_cap_generation_matches(current_generation, current_generation - 1U));
#if defined(BHARAT_ENABLE_LEGACY_CAP_TESTS) && BHARAT_ENABLE_LEGACY_CAP_TESTS
    assert(bh_cap_generation_matches(current_generation, 0U));
#else
    assert(!bh_cap_generation_matches(current_generation, 0U));
#endif
    return 0;
}
