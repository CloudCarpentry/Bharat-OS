#include "backend.h"
#include <string.h>

bh_status_t bh_console_write(const char *message) { return message ? bh_backend_console_write(message) : BH_ERR_INVALID_ARGUMENT; }
bh_status_t bh_log(const char *message) { return bh_console_write(message); }
bh_status_t bh_system_info(bh_system_info_t *info) {
    if (!info) return BH_ERR_INVALID_ARGUMENT;
    memset(info, 0, sizeof(*info));
    info->abi_version = BH_SDK_ABI_VERSION;
    info->struct_size = sizeof(*info);
    return bh_backend_system_info(info);
}
bh_status_t bh_time_now(uint64_t *nanoseconds) { return nanoseconds ? bh_backend_time_now(nanoseconds) : BH_ERR_INVALID_ARGUMENT; }
bh_status_t bh_sleep(uint64_t nanoseconds) { return bh_backend_sleep(nanoseconds); }
void bh_exit(int32_t status) { bh_backend_exit(status); }
