#pragma once

#include <stdint.h>

uint64_t hal_timer_read_ns(void);
void hal_timer_program_oneshot_ns(uint64_t ns_from_now);
