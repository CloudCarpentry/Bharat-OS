/* SPDX-License-Identifier: MIT */
#include "bharat_lvgl.h"
#include "lvgl.h"
#include <bharat/bh_native.h>
#include <stdint.h>
#include <uapi/time/time.h>

#define BHARAT_LVGL_MAX_POLL_MS UINT32_C(1000)

uint32_t bharat_lvgl_now_ms(void) {
  bh_time_t now_ns = 0;

  if (bh_time_get(BH_CLOCK_MONOTONIC, &now_ns) != 0) {
    return 0;
  }

  /* LVGL defines its tick source as a wrapping 32-bit millisecond counter. */
  return (uint32_t)(now_ns / BH_NS_PER_MS);
}

void bharat_lvgl_wait_ms(uint32_t delay_ms) {
  bh_time_t start_ns = 0;
  bh_time_t now_ns = 0;
  bh_time_t delay_ns;

  if (delay_ms == 0 || bh_time_get(BH_CLOCK_MONOTONIC, &start_ns) != 0) {
    return;
  }

  /*
   * Native sleep is not yet available in the runtime. Keep this bring-up
   * fallback bounded even when LVGL reports that no timer is ready.
   */
  if (delay_ms > BHARAT_LVGL_MAX_POLL_MS) {
    delay_ms = BHARAT_LVGL_MAX_POLL_MS;
  }
  delay_ns = (bh_time_t)delay_ms * BH_NS_PER_MS;

  do {
    if (bh_time_get(BH_CLOCK_MONOTONIC, &now_ns) != 0 || now_ns < start_ns) {
      return;
    }
  } while ((now_ns - start_ns) < delay_ns);
}

void bharat_lvgl_tick_init(void) { lv_tick_set_cb(bharat_lvgl_now_ms); }
