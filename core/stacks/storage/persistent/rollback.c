#include "bharat/stacks/storage/persistent/rollback.h"

rollback_protection_level_t g_test_rollback_level = ROLLBACK_PROTECTION_NONE;
uint64_t g_test_rollback_epoch = 1ULL;
