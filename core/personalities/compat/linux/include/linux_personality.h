#ifndef LINUX_PERSONALITY_H
#define LINUX_PERSONALITY_H

#include "personality_ops.h"
#include "personality/personality_types.h"
#include "trap/syscall_context.h"

const personality_ops_t *personality_linux_get_ops(void);
const bh_personality_syscall_table_t *personality_linux_get_table(void);

#endif // LINUX_PERSONALITY_H
