#ifndef BHARAT_SDK_INTERNAL_BACKEND_H
#define BHARAT_SDK_INTERNAL_BACKEND_H
#include <bharat/capability.h>
#include <bharat/device.h>
#include <bharat/ipc.h>
#include <bharat/os.h>
#include <bharat/process.h>
bh_status_t bh_backend_console_write(const char *message);
bh_status_t bh_backend_system_info(bh_system_info_t *info);
bh_status_t bh_backend_time_now(uint64_t *nanoseconds);
bh_status_t bh_backend_sleep(uint64_t nanoseconds);
void bh_backend_exit(int32_t status);
#endif
