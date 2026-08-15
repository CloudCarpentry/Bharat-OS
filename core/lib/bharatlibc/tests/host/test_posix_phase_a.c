#include <bharat/bsys/backend.h>
#include <standard/assert.h>
#include <standard/errno.h>
#include <standard/stdint.h>
#include <standard/stdlib.h>
#include <standard/string.h>
#include <standard/time.h>
#include <standard/unistd.h>

static union {
  uint64_t alignment;
  uint8_t bytes[4096];
} heap;
static uint32_t last_handle;

static int32_t mock_write(uint32_t handle, const void *buffer, uint32_t length,
                          uint32_t *written) {
  last_handle = handle;
  assert(buffer != NULL);
  *written = length;
  return 0;
}

static int32_t mock_read(uint32_t handle, void *buffer, uint32_t capacity,
                         uint32_t *received) {
  last_handle = handle;
  if (capacity != 0U)
    ((uint8_t *)buffer)[0] = 0x5aU;
  *received = capacity != 0U;
  return 0;
}

static int32_t mock_close(uint32_t handle) {
  last_handle = handle;
  return handle == 9U ? 0 : -9;
}

static int32_t mock_clock(uint32_t clock_id, bh_bsys_timespec_t *out_time) {
  assert(clock_id == CLOCK_MONOTONIC);
  out_time->tv_sec = 12U;
  out_time->tv_nsec = 34U;
  return 0;
}

static int32_t mock_sleep(const bh_bsys_timespec_t *request,
                          bh_bsys_timespec_t *remaining) {
  assert(request->tv_sec == 0U && request->tv_nsec == 50U);
  if (remaining)
    *remaining = (bh_bsys_timespec_t){0, 0};
  return 0;
}

static int32_t mock_pid(uint32_t *out_process_id) {
  *out_process_id = 42U;
  return 0;
}

static int32_t mock_isatty(uint32_t handle, uint32_t *out_is_tty) {
  *out_is_tty = handle == 1U;
  return 0;
}

static int32_t mock_heap(uintptr_t *out_base, uint32_t *out_size) {
  *out_base = (uintptr_t)heap.bytes;
  *out_size = sizeof(heap.bytes);
  return 0;
}

int main(void) {
  const bh_bsys_backend_ops_t backend = {
      .abi_version = 1,
      .structure_size = sizeof(backend),
      .write = mock_write,
      .read = mock_read,
      .close = mock_close,
      .clock_gettime = mock_clock,
      .nanosleep = mock_sleep,
      .process_id = mock_pid,
      .isatty = mock_isatty,
      .heap_region = mock_heap,
  };
  uint8_t byte = 0;
  struct timespec time_value;
  struct timespec delay = {0, 50};

  bh_bsys_register_backend(&backend);
  assert(write(1, "x", 1) == 1 && last_handle == 1U);
  assert(read(0, &byte, 1) == 1 && byte == 0x5aU && last_handle == 0U);
  assert(close(9) == 0 && close(8) == -1 && errno == EBADF);
  assert(clock_gettime(CLOCK_MONOTONIC, &time_value) == 0);
  assert(time_value.tv_sec == 12 && time_value.tv_nsec == 34);
  assert(nanosleep(&delay, NULL) == 0);
  assert(getpid() == 42);
  assert(isatty(1) == 1 && isatty(2) == 0);

  void *first = malloc(64U);
  void *second = malloc(64U);
  assert(first != NULL && second != NULL && first != second);
  memset(first, 0xa5, 64U);
  memcpy(second, first, 64U);
  assert(memcmp(first, second, 64U) == 0);
  memmove((uint8_t *)second + 1, second, 63U);
  assert(((uint8_t *)second)[1] == 0xa5U);
  free(first);
  free(second);

  assert(clock_gettime(99, &time_value) == -1 && errno == EINVAL);
  delay.tv_nsec = 1000000000;
  assert(nanosleep(&delay, NULL) == -1 && errno == EINVAL);
  assert(read(-1, &byte, 1) == -1 && errno == EINVAL);
  return 0;
}
