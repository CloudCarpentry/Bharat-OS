#include <bharat/bsys/backend.h>
#include <bharat/libc/status.h>
#include <standard/errno.h>
#include <standard/stdint.h>
#include <standard/sys/types.h>
#include <standard/time.h>
#include <standard/unistd.h>

#define BH_POSIX_BACKEND_MAX_IO UINT32_MAX
#define BH_POSIX_NSEC_PER_SEC ((uint64_t)1000000000U)

static int fail_status(int32_t status) {
  errno = bh_status_to_errno(status);
  return -1;
}

static const bh_bsys_backend_ops_t *backend_or_fail(void) {
  const bh_bsys_backend_ops_t *backend = bh_bsys_get_backend();
  if (!backend)
    errno = ENOSYS;
  return backend;
}

ssize_t read(int descriptor, void *buffer, size_t capacity) {
  const bh_bsys_backend_ops_t *backend = backend_or_fail();
  uint32_t received = 0;
  int32_t status;

  if (!backend || !backend->read)
    return -1;
  if (descriptor < 0 || (!buffer && capacity != 0U) ||
      capacity > BH_POSIX_BACKEND_MAX_IO) {
    errno = EINVAL;
    return -1;
  }
  status = backend->read((uint32_t)descriptor, buffer, (uint32_t)capacity,
                         &received);
  return status == 0 ? (ssize_t)received : (ssize_t)fail_status(status);
}

ssize_t write(int descriptor, const void *buffer, size_t length) {
  const bh_bsys_backend_ops_t *backend = backend_or_fail();
  uint32_t written = 0;
  int32_t status;

  if (!backend || !backend->write)
    return -1;
  if (descriptor < 0 || (!buffer && length != 0U) ||
      length > BH_POSIX_BACKEND_MAX_IO) {
    errno = EINVAL;
    return -1;
  }
  status =
      backend->write((uint32_t)descriptor, buffer, (uint32_t)length, &written);
  return status == 0 ? (ssize_t)written : (ssize_t)fail_status(status);
}

int close(int descriptor) {
  const bh_bsys_backend_ops_t *backend = backend_or_fail();
  int32_t status;

  if (!backend || !backend->close)
    return -1;
  if (descriptor < 0) {
    errno = EBADF;
    return -1;
  }
  status = backend->close((uint32_t)descriptor);
  return status == 0 ? 0 : fail_status(status);
}

int clock_gettime(clockid_t clock_id, struct timespec *time_value) {
  const bh_bsys_backend_ops_t *backend = backend_or_fail();
  bh_bsys_timespec_t value = {0, 0};
  int32_t status;

  if (!backend || !backend->clock_gettime)
    return -1;
  if (!time_value ||
      (clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC)) {
    errno = EINVAL;
    return -1;
  }
  status = backend->clock_gettime((uint32_t)clock_id, &value);
  if (status != 0)
    return fail_status(status);
  if (value.tv_sec > INT64_MAX || value.tv_nsec >= BH_POSIX_NSEC_PER_SEC) {
    errno = EIO;
    return -1;
  }
  time_value->tv_sec = (time_t)value.tv_sec;
  time_value->tv_nsec = (int64_t)value.tv_nsec;
  return 0;
}

int nanosleep(const struct timespec *request, struct timespec *remaining) {
  const bh_bsys_backend_ops_t *backend = backend_or_fail();
  bh_bsys_timespec_t requested;
  bh_bsys_timespec_t remainder = {0, 0};
  int32_t status;

  if (!backend || !backend->nanosleep)
    return -1;
  if (!request || request->tv_sec < 0 || request->tv_nsec < 0 ||
      (uint64_t)request->tv_nsec >= BH_POSIX_NSEC_PER_SEC) {
    errno = EINVAL;
    return -1;
  }
  requested.tv_sec = (uint64_t)request->tv_sec;
  requested.tv_nsec = (uint64_t)request->tv_nsec;
  status = backend->nanosleep(&requested, remaining ? &remainder : 0);
  if (remaining) {
    remaining->tv_sec = (time_t)remainder.tv_sec;
    remaining->tv_nsec = (int64_t)remainder.tv_nsec;
  }
  return status == 0 ? 0 : fail_status(status);
}

pid_t getpid(void) {
  const bh_bsys_backend_ops_t *backend = backend_or_fail();
  uint32_t process_id = 0;
  int32_t status;

  if (!backend || !backend->process_id)
    return -1;
  status = backend->process_id(&process_id);
  if (status != 0)
    return (pid_t)fail_status(status);
  if (process_id > INT32_MAX) {
    errno = EOVERFLOW;
    return -1;
  }
  return (pid_t)process_id;
}

int isatty(int descriptor) {
  const bh_bsys_backend_ops_t *backend = backend_or_fail();
  uint32_t is_tty = 0;
  int32_t status;

  if (!backend || !backend->isatty)
    return 0;
  if (descriptor < 0) {
    errno = EBADF;
    return 0;
  }
  status = backend->isatty((uint32_t)descriptor, &is_tty);
  if (status != 0) {
    fail_status(status);
    return 0;
  }
  return is_tty != 0U;
}
