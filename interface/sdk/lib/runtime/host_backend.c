#define _POSIX_C_SOURCE 200809L
#include "backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

bh_status_t bh_backend_console_write(const char *message) { return fputs(message, stdout) < 0 ? BH_ERR_IO : BH_OK; }
bh_status_t bh_backend_system_info(bh_system_info_t *info) {
    long cpus = sysconf(_SC_NPROCESSORS_ONLN);
    long page = sysconf(_SC_PAGESIZE);
    info->cpu_count = cpus > 0 ? (uint32_t)cpus : 1u;
    info->page_size = page > 0 ? (uint32_t)page : 0u;
    memcpy(info->os_name, "Bharat-OS hosted", sizeof("Bharat-OS hosted"));
    memcpy(info->os_version, "0.1", sizeof("0.1"));
    return BH_OK;
}
bh_status_t bh_backend_time_now(uint64_t *nanoseconds) {
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) return BH_ERR_IO;
    *nanoseconds = (uint64_t)value.tv_sec * UINT64_C(1000000000) + (uint64_t)value.tv_nsec;
    return BH_OK;
}
bh_status_t bh_backend_sleep(uint64_t nanoseconds) {
    struct timespec requested = {(time_t)(nanoseconds / UINT64_C(1000000000)), (long)(nanoseconds % UINT64_C(1000000000))};
    return nanosleep(&requested, NULL) == 0 ? BH_OK : BH_ERR_IO;
}
void bh_backend_exit(int32_t status) { exit(status); }
