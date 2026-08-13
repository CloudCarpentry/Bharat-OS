#include <bharat/bsys/backend.h>
#include <bharat/uapi/syscall/bh_syscall_numbers.h>
#include <bharat/uapi/time/time.h>
#include <standard/stddef.h>

extern long bh_syscall(long sysno, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long arg6);

/* The Bharat skeleton backend is compiled unconditionally so it can be
 * registered and tested on host, or utilized as a default target skeleton.
 */

static int32_t bharat_write(uint32_t handle, const void *buffer,
                            uint32_t length, uint32_t *written) {
  (void)handle;
  (void)buffer;
  (void)length;
  (void)written;
  return -38; /* -SYS_ENOSYS */
}

static int32_t bharat_read(uint32_t handle, void *buffer, uint32_t capacity,
                           uint32_t *received) {
  (void)handle;
  (void)buffer;
  (void)capacity;
  (void)received;
  return -38; /* -SYS_ENOSYS */
}

static int32_t bharat_close(uint32_t handle) {
  (void)handle;
  return -38; /* -SYS_ENOSYS */
}

static int32_t bharat_clock_gettime(uint32_t clock_id,
                                    bh_bsys_timespec_t *out_time) {
  uint64_t now_ns = 0;
  long status;

  if (clock_id != BH_CLOCK_MONOTONIC || !out_time)
    return -22; /* -SYS_EINVAL */
  status = bh_syscall(BH_SYS_TIME_GET, (long)clock_id, (long)&now_ns, 0, 0, 0,
                      0);
  if (status != 0)
    return (int32_t)status;
  out_time->tv_sec = now_ns / BH_NS_PER_SEC;
  out_time->tv_nsec = now_ns % BH_NS_PER_SEC;
  return 0;
}

static int32_t bharat_nanosleep(const bh_bsys_timespec_t *request,
                                bh_bsys_timespec_t *remaining) {
  (void)request;
  (void)remaining;
  return -38; /* -SYS_ENOSYS */
}

static int32_t bharat_sleep_until(uint64_t deadline_ticks) {
  (void)deadline_ticks;
  return -38; /* -SYS_ENOSYS */
}

static int32_t bharat_process_id(uint32_t *out_process_id) {
  (void)out_process_id;
  return -38; /* -SYS_ENOSYS */
}

static int32_t bharat_isatty(uint32_t handle, uint32_t *out_is_tty) {
  (void)handle;
  (void)out_is_tty;
  return -38; /* -SYS_ENOSYS */
}

static int32_t bharat_heap_region(uintptr_t *out_base, uint32_t *out_size) {
  (void)out_base;
  (void)out_size;
  return -38; /* -SYS_ENOSYS */
}

static void bharat_process_exit(int32_t status) {
  (void)status;
  while (1) {
    __asm__ __volatile__("");
  }
}

static const bh_bsys_backend_ops_t g_bharat_ops = {
    .abi_version = 1,
    .structure_size = sizeof(bh_bsys_backend_ops_t),
    .write = bharat_write,
    .read = bharat_read,
    .close = bharat_close,
    .clock_gettime = bharat_clock_gettime,
    .sleep_until = bharat_sleep_until,
    .heap_region = bharat_heap_region,
    .process_exit = bharat_process_exit,
    .nanosleep = bharat_nanosleep,
    .process_id = bharat_process_id,
    .isatty = bharat_isatty};

void bh_bsys_init_bharat_backend(void) {
  bh_bsys_register_backend(&g_bharat_ops);
}
