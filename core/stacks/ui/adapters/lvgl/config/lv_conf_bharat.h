/* SPDX-License-Identifier: MIT */
#ifndef LV_CONF_BHARAT_H
#define LV_CONF_BHARAT_H

#include <stdint.h>

#define LV_USE_OS   0
#define LV_USE_LOG  1
#define LV_USE_ASSERT_NULL      1
#define LV_USE_ASSERT_MALLOC    1
#define LV_USE_ASSERT_STYLE     1
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

#endif /*LV_CONF_BHARAT_H*/
