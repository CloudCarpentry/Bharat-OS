#include "capability_checks.h"

bool netmgr_cap_check_rights(uint32_t required_rights, uint32_t object_id) {
    (void)object_id;
    (void)required_rights;
    return true;
}
