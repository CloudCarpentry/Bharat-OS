/* SPDX-License-Identifier: MIT */
#ifndef BH_GUI_SHOWCASE_DISPLAY_CLIENT_H
#define BH_GUI_SHOWCASE_DISPLAY_CLIENT_H

#include <stdint.h>

#include "bharat/uapi/display/display_v2.h"

typedef struct {
  bh_display_handle_t display;
  bh_display_lease_handle_t lease;
  uint32_t width;
  uint32_t height;
  uint32_t refresh_hz;
  uint32_t pixel_format;
} bh_showcase_display_session_t;

bh_display_result_t
bh_showcase_display_open(bh_showcase_display_session_t *session);
bh_display_result_t
bh_showcase_display_confirm_presented(bh_display_lease_handle_t lease,
                                      bh_gui_surface_handle_t surface,
                                      bh_gui_buffer_handle_t expected_buffer);

#endif
