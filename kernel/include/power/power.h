#ifndef BHARAT_OS_POWER_H
#define BHARAT_OS_POWER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Forward declarations */
typedef struct power_domain power_domain_t;

/* Generic power capability flags */
#define POWER_CAP_SUPPORTED (1 << 0)
#define POWER_CAP_CAN_SUSPEND (1 << 1)
#define POWER_CAP_CAN_RETENTION (1 << 2)

#endif /* BHARAT_OS_POWER_H */
