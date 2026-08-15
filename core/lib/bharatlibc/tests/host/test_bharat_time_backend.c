#include <bharat/bsys/backend.h>
#include <bharat/uapi/syscall/bh_syscall_numbers.h>
#include <bharat/uapi/time/time.h>
#include <standard/assert.h>
#include <standard/stdint.h>

static long observed_number;
static long observed_clock;

long bh_syscall(long number, long arg1, long arg2, long arg3, long arg4,
                long arg5, long arg6) {
  uint64_t *out_ns = (uint64_t *)arg2;
  (void)arg3; (void)arg4; (void)arg5; (void)arg6;
  observed_number = number;
  observed_clock = arg1;
  *out_ns = UINT64_C(12000000034);
  return 0;
}

void bh_bsys_init_bharat_backend(void);

int main(void) {
  const bh_bsys_backend_ops_t *backend;
  bh_bsys_timespec_t value = {0, 0};
  bh_bsys_init_bharat_backend();
  backend = bh_bsys_get_backend();
  assert(backend != 0 && backend->clock_gettime != 0);
  assert(backend->clock_gettime(BH_CLOCK_MONOTONIC, &value) == 0);
  assert(observed_number == BH_SYS_TIME_GET);
  assert(observed_clock == BH_CLOCK_MONOTONIC);
  assert(value.tv_sec == 12U && value.tv_nsec == 34U);
  assert(backend->clock_gettime(BH_CLOCK_REALTIME, &value) == -22);
  return 0;
}
