/* SPDX-License-Identifier: MIT */
#ifndef LV_CONF_BHARAT_H
#define LV_CONF_BHARAT_H

#include <stdint.h>

#define LV_USE_OS   0
#define LV_USE_LOG  0
#define LV_USE_ASSERT_NULL      0
#define LV_USE_ASSERT_MALLOC    0
#define LV_USE_ASSERT_STYLE     0
#define LV_USE_DRAW_SW          1

#define LV_USE_LABEL            1
#define LV_USE_BUTTON           1
#define LV_USE_BAR              1
#define LV_USE_LIST             1
#define LV_USE_TABLE            1
#define LV_USE_MSGBOX           1
#define LV_USE_FLEX             1
#define LV_USE_GRID             1
#define LV_USE_DROPDOWN         1
#define LV_USE_IMAGE            1
#define LV_USE_ARC              1

#define LV_COLOR_DEPTH          32

/* Use custom memory allocators eventually, standard for now */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

/* Theme configuration */
#define LV_USE_THEME_DEFAULT    0

/* Enable empty stubs to make compiling simpler vs hacking headers */
#define LV_USE_FREETYPE         0
#define LV_USE_OPENGLES         0
#define LV_USE_SDL              0
#define LV_USE_X11              0
#define LV_USE_WAYLAND          0

// Ensure version skips
#define LV_CONF_SKIP            1

#define LV_USE_NUTTX 0
#define LV_USE_WINDOWS 0
#define LV_USE_LINUX_DRM 0
#define LV_USE_LINUX_FBDEV 0

#define LV_USE_TINY_TTF 0
#define LV_USE_FFMPEG 0
#define LV_USE_BIN_DECODER 0
#define LV_USE_RLOTTIE 0

#endif /*LV_CONF_BHARAT_H*/
