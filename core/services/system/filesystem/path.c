#include "fs/path.h"
#include "kernel/status.h"

// PHASE1_COMPAT_SHIM: Path validation logic moved to services/system/filesystem/.

int vfs_validate_path_rights(const char* path, uint32_t required_rights, capability_t* cap) {
    (void)path;
    (void)required_rights;
    (void)cap;
    return K_ERR_REQUIRES_FS_SERVICE;
}
