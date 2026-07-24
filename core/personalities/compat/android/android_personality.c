#include "android_personality.h"
#include "bh_personality_registry.h"
#include "bh_personality.h"
#include <stddef.h>

extern const personality_ops_t *personality_linux_get_ops(void);

const personality_ops_t *personality_android_get_ops(void) {
    // Reuse Linux ops for now
    return personality_linux_get_ops();
}

void android_personality_init(void) {
    bh_personality_registry_register(BH_PERSONALITY_ANDROID, personality_android_get_ops());
}
