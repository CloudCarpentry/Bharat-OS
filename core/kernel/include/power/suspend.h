#ifndef BHARAT_OS_SUSPEND_H
#define BHARAT_OS_SUSPEND_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SUSPEND_STATE_FREEZE,
    SUSPEND_STATE_STANDBY,
    SUSPEND_STATE_MEM,
    SUSPEND_STATE_DISK
} suspend_state_t;

typedef struct suspend_ops {
    int (*valid)(suspend_state_t state);
    int (*begin)(suspend_state_t state);
    int (*prepare)(void);
    int (*prepare_late)(void);
    int (*enter)(suspend_state_t state);
    void (*wake)(void);
    void (*finish)(void);
    void (*end)(void);
} suspend_ops_t;

void suspend_set_ops(const suspend_ops_t *ops);
int suspend_system(suspend_state_t state);

#endif /* BHARAT_OS_SUSPEND_H */
