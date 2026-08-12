#include <bharat/device.h>
#include <bharat/os.h>
int main(void) { bh_device_info_t info = {.struct_size = sizeof(info)}; bh_status_t status = bh_device_find(0, 0, &info); if (status == BH_ERR_UNSUPPORTED) return bh_log("devices: unavailable on hosted v0.1 backend\n") != BH_OK; return status != BH_OK; }
