#define _POSIX_C_SOURCE 200809L
#include <bharat/bsys/backend.h>
#include <standard/stddef.h>

#ifdef BHARATLIBC_HOST_MODE

#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static int32_t host_write(uint32_t handle, const void *buffer, uint32_t length,
                          uint32_t *written) {
  ssize_t rc = write((int)handle, buffer, (size_t)length);
  if (rc < 0)
    return -errno;
  if (written)
    *written = (uint32_t)rc;
  return 0;
}

static int32_t host_read(uint32_t handle, void *buffer, uint32_t capacity,
                         uint32_t *received) {
  ssize_t rc = read((int)handle, buffer, (size_t)capacity);
  if (rc < 0)
    return -errno;
  if (received)
    *received = (uint32_t)rc;
  return 0;
}

static int32_t host_close(uint32_t handle) {
  int rc = close((int)handle);
  return (rc == 0) ? 0 : -errno;
}

static int32_t host_clock_gettime(uint32_t clock_id,
                                  bh_bsys_timespec_t *out_time) {
  struct timespec ts;
  int rc = clock_gettime((clockid_t)clock_id, &ts);
  if (rc == 0 && out_time) {
    out_time->tv_sec = ts.tv_sec;
    out_time->tv_nsec = ts.tv_nsec;
    return 0;
  }
  return -errno;
}

static int32_t host_nanosleep(const bh_bsys_timespec_t *request,
                              bh_bsys_timespec_t *remaining) {
  struct timespec req = {(time_t)request->tv_sec, (long)request->tv_nsec};
  struct timespec rem = {0, 0};
  if (nanosleep(&req, &rem) == 0)
    return 0;
  if (remaining) {
    remaining->tv_sec = (uint64_t)rem.tv_sec;
    remaining->tv_nsec = (uint64_t)rem.tv_nsec;
  }
  return -errno;
}

static int32_t host_sleep_until(uint64_t deadline_ticks) {
  struct timespec now;
  bh_bsys_timespec_t delay;

  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    return -errno;
  const uint64_t nsec_per_sec = (uint64_t)1000000000U;
  uint64_t now_ns = (uint64_t)now.tv_sec * nsec_per_sec + (uint64_t)now.tv_nsec;
  if (deadline_ticks <= now_ns)
    return 0;
  delay.tv_sec = (deadline_ticks - now_ns) / nsec_per_sec;
  delay.tv_nsec = (deadline_ticks - now_ns) % nsec_per_sec;
  return host_nanosleep(&delay, NULL);
}

static int32_t host_process_id(uint32_t *out_process_id) {
  pid_t pid = getpid();
  if (pid < 0)
    return -errno;
  *out_process_id = (uint32_t)pid;
  return 0;
}

static int32_t host_isatty(uint32_t handle, uint32_t *out_is_tty) {
  int rc = isatty((int)handle);
  if (rc == 0 && errno != 0)
    return -errno;
  *out_is_tty = rc != 0;
  return 0;
}

static union {
  uint64_t alignment;
  uint8_t bytes[1024 * 1024];
} host_heap; /* Process-local 1 MiB test heap. */

static int32_t host_heap_region(uintptr_t *out_base, uint32_t *out_size) {
  if (out_base)
    *out_base = (uintptr_t)host_heap.bytes;
  if (out_size)
    *out_size = sizeof(host_heap.bytes);
  return 0;
}

static void host_process_exit(int32_t status) { exit(status); }

static const bh_bsys_backend_ops_t g_host_ops = {
    .abi_version = 1,
    .structure_size = sizeof(bh_bsys_backend_ops_t),
    .write = host_write,
    .read = host_read,
    .close = host_close,
    .clock_gettime = host_clock_gettime,
    .sleep_until = host_sleep_until,
    .heap_region = host_heap_region,
    .process_exit = host_process_exit,
    .nanosleep = host_nanosleep,
    .process_id = host_process_id,
    .isatty = host_isatty};

void bh_bsys_init_host_backend(void) { bh_bsys_register_backend(&g_host_ops); }

#else

void bh_bsys_init_host_backend(void) {}

#endif
