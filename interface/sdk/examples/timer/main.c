#include <bharat/os.h>
int main(void) { uint64_t before, after; if (bh_time_now(&before) != BH_OK || bh_sleep(1000000u) != BH_OK || bh_time_now(&after) != BH_OK) return 1; return after < before; }
