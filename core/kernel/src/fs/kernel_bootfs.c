#include <bharat/kernel/fs/bootfs.h>
#include "kernel/status.h"

kstatus_t bh_boot_resource_lookup(const char *name, bh_boot_resource_t *res) {
    (void)name; (void)res;
    // Minimal read-only lookup for boot resources
    return K_ERR_NOT_FOUND;
}
