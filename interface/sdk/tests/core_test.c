#include <bharat/capability.h>
#include <bharat/device.h>
#include <bharat/ipc.h>
#include <bharat/os.h>
#include <bharat/process.h>
#include <string.h>
int main(void) {
    bh_system_info_t info; uint64_t first, second;
    if (bh_system_info(0) != BH_ERR_INVALID_ARGUMENT) return 1;
    if (bh_system_info(&info) != BH_OK || info.abi_version != BH_SDK_ABI_VERSION || strcmp(info.os_name, "Bharat-OS hosted")) return 2;
    if (bh_time_now(&first) != BH_OK || bh_time_now(&second) != BH_OK || second < first) return 3;
    if (bh_device_find(0, 0, 0) != BH_ERR_UNSUPPORTED) return 4;
    if (bh_endpoint_open("test", 0) != BH_ERR_UNSUPPORTED) return 5;
    if (bh_cap_close(1) != BH_ERR_UNSUPPORTED) return 6;
    if (bh_process_wait(1, 0, 0) != BH_ERR_UNSUPPORTED) return 7;
    return 0;
}
