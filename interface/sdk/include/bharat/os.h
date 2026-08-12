#ifndef BHARAT_SDK_OS_H
#define BHARAT_SDK_OS_H

#include <bharat/types.h>

typedef struct bh_system_info {
    uint32_t abi_version;
    uint32_t struct_size;
    uint32_t cpu_count;
    uint32_t page_size;
    char os_name[32];
    char os_version[16];
} bh_system_info_t;

bh_status_t bh_console_write(const char *message);
bh_status_t bh_system_info(bh_system_info_t *info);
bh_status_t bh_time_now(uint64_t *nanoseconds);
bh_status_t bh_sleep(uint64_t nanoseconds);
bh_status_t bh_log(const char *message);
void bh_exit(int32_t status);

#endif
