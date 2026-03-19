#ifndef NETMGR_CAPABILITY_CHECKS_H
#define NETMGR_CAPABILITY_CHECKS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    NETMGR_CAP_READ = (1 << 0),
    NETMGR_CAP_WRITE = (1 << 1),
    NETMGR_CAP_ADMIN = (1 << 2),
} netmgr_cap_rights_t;

bool netmgr_cap_check_rights(uint32_t required_rights, uint32_t object_id);

#endif // NETMGR_CAPABILITY_CHECKS_H
