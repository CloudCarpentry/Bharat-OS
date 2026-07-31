#include <bharat/bsys/backend.h>
#include <standard/stddef.h>

#ifdef BHARATLIBC_HOST_MODE

#include <unistd.h>
#include <time.h>
#include <stdlib.h>

static int32_t host_write(uint32_t handle, const void *buffer, uint32_t length, uint32_t *written) {
    ssize_t rc = write((int)handle, buffer, (size_t)length);
    if (rc < 0) return -1;
    if (written) *written = (uint32_t)rc;
    return 0;
}

static int32_t host_read(uint32_t handle, void *buffer, uint32_t capacity, uint32_t *received) {
    ssize_t rc = read((int)handle, buffer, (size_t)capacity);
    if (rc < 0) return -1;
    if (received) *received = (uint32_t)rc;
    return 0;
}

static int32_t host_close(uint32_t handle) {
    int rc = close((int)handle);
    return (rc == 0) ? 0 : -1;
}

static int32_t host_clock_gettime(uint32_t clock_id, bh_bsys_timespec_t *out_time) {
    struct timespec ts;
    int rc = clock_gettime((clockid_t)clock_id, &ts);
    if (rc == 0 && out_time) {
        out_time->tv_sec = ts.tv_sec;
        out_time->tv_nsec = ts.tv_nsec;
        return 0;
    }
    return -1;
}

static int32_t host_sleep_until(uint64_t deadline_ticks) {
    struct timespec req = {0, 1000000}; /* sleep for 1ms */
    nanosleep(&req, NULL);
    return 0;
}

static uint8_t host_heap[1024 * 1024]; /* 1MB static host heap */

static int32_t host_heap_region(uintptr_t *out_base, uint32_t *out_size) {
    if (out_base) *out_base = (uintptr_t)host_heap;
    if (out_size) *out_size = sizeof(host_heap);
    return 0;
}

static void host_process_exit(int32_t status) {
    exit(status);
}

static const bh_bsys_backend_ops_t g_host_ops = {
    .abi_version = 1,
    .structure_size = sizeof(bh_bsys_backend_ops_t),
    .write = host_write,
    .read = host_read,
    .close = host_close,
    .clock_gettime = host_clock_gettime,
    .sleep_until = host_sleep_until,
    .heap_region = host_heap_region,
    .process_exit = host_process_exit
};

void bh_bsys_init_host_backend(void) {
    bh_bsys_register_backend(&g_host_ops);
}

#else

void bh_bsys_init_host_backend(void) {}

#endif
