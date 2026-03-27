#include "personality/personality_hooks.h"
#include <stddef.h>

static const personality_ops_t* current_personality_ops = NULL;

void personality_registry_init(void) {
    current_personality_ops = NULL;
}

void personality_register_ops(const personality_ops_t *ops) {
    current_personality_ops = ops;
}

const personality_ops_t* personality_get_current_ops(void) {
    return current_personality_ops;
}
