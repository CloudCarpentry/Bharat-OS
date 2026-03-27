#ifndef BHARAT_PERSONALITY_HOOKS_H
#define BHARAT_PERSONALITY_HOOKS_H

#include "personality_ops.h"

void personality_registry_init(void);
void personality_register_ops(const personality_ops_t *ops);
const personality_ops_t* personality_get_current_ops(void);

#endif
