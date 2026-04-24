#include "debug/mm_invariants.h"
#include "console/console_core.h"

struct mm_debug_stats mm_stats = {0};

void mm_debug_dump(void) {
    console_log(CONSOLE_LEVEL_INFO, "=== MM Debug Stats ===\n");
    console_log(CONSOLE_LEVEL_INFO, "aspace_create calls:       %llu\n", (unsigned long long)mm_stats.aspace_create_calls);
    console_log(CONSOLE_LEVEL_INFO, "aspace_create failures:    %llu\n", (unsigned long long)mm_stats.aspace_create_failures);
    console_log(CONSOLE_LEVEL_INFO, "aspace rejected by profile: %llu\n", (unsigned long long)mm_stats.aspace_rejected_by_profile);
    console_log(CONSOLE_LEVEL_INFO, "======================\n");
}
