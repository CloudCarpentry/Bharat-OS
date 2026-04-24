#pragma once

#include "console_base_types.h"

#define CON_CAP_EARLY_BOOT         ((console_caps_t)1u << 0)
#define CON_CAP_RUNTIME            ((console_caps_t)1u << 1)
#define CON_CAP_PANIC_SAFE         ((console_caps_t)1u << 2)
#define CON_CAP_WRITE_POLL         ((console_caps_t)1u << 3)
#define CON_CAP_WRITE_IRQ          ((console_caps_t)1u << 4)
#define CON_CAP_WRITE_DMA          ((console_caps_t)1u << 5)
#define CON_CAP_COLOR              ((console_caps_t)1u << 6)
#define CON_CAP_CURSOR             ((console_caps_t)1u << 7)
#define CON_CAP_CLEAR              ((console_caps_t)1u << 8)
#define CON_CAP_SCROLL             ((console_caps_t)1u << 9)
#define CON_CAP_GEOMETRY           ((console_caps_t)1u << 10)
#define CON_CAP_UTF8               ((console_caps_t)1u << 11)
#define CON_CAP_VT100              ((console_caps_t)1u << 12)
#define CON_CAP_FRAMEBUFFER_TEXT   ((console_caps_t)1u << 13)
#define CON_CAP_INPUT_POLL         ((console_caps_t)1u << 14)
#define CON_CAP_REPLAY_SAFE        ((console_caps_t)1u << 15)
#define CON_CAP_CRASH_PRESERVE     ((console_caps_t)1u << 16)
#define CON_CAP_VISIBLE_SINK       ((console_caps_t)1u << 17)
#define CON_CAP_STORAGE_SINK       ((console_caps_t)1u << 18)
