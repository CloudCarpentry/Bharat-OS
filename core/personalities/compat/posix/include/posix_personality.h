#ifndef POSIX_PERSONALITY_H
#define POSIX_PERSONALITY_H

#include "personality_ops.h"
#include "personality/personality_types.h"
#include "trap/syscall_context.h"

const personality_ops_t *personality_posix_get_ops(void);
const bh_personality_syscall_table_t *personality_posix_get_table(void);
void posix_personality_init(void);

#endif // POSIX_PERSONALITY_H
