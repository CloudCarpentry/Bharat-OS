#ifndef BHARAT_INTERFACE_PERSONALITY_H
#define BHARAT_INTERFACE_PERSONALITY_H

#include "trap/syscall_context.h"
#include "personality_ops.h"

/**
 * Standard interface for personalities to export their configuration.
 */

// Native
const bh_personality_syscall_table_t *personality_native_get_table(void);
const personality_ops_t *personality_native_get_ops(void);

// Linux
const bh_personality_syscall_table_t *personality_linux_get_table(void);
const personality_ops_t *personality_linux_get_ops(void);

// Android
const bh_personality_syscall_table_t *personality_android_get_table(void);
const personality_ops_t *personality_android_get_ops(void);

// Windows
const bh_personality_syscall_table_t *personality_windows_get_table(void);
const personality_ops_t *personality_windows_get_ops(void);

#endif /* BHARAT_INTERFACE_PERSONALITY_H */
