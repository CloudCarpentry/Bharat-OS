#ifndef BHARAT_CONSOLE_CAPS_H
#define BHARAT_CONSOLE_CAPS_H

#include <stdint.h>

/*
 * Console Backend Capabilities
 * Defines what a registered console backend is capable of handling.
 */

// I/O Methods
#define CON_CAP_WRITE_POLLING     (1ULL << 0)  // Simple synchronous polling
#define CON_CAP_WRITE_IRQ         (1ULL << 1)  // Interrupt-driven asynchronous writes
#define CON_CAP_WRITE_DMA         (1ULL << 2)  // DMA-accelerated writes

// Context Safety
#define CON_CAP_CRASH_SAFE        (1ULL << 3)  // Can safely write during panic/crash
#define CON_CAP_EARLY_BOOT        (1ULL << 4)  // Can be used before MMU/scheduler setup

// Text Rendering Capabilities
#define CON_CAP_VT100             (1ULL << 5)  // Understands basic VT100 escapes
#define CON_CAP_COLOR             (1ULL << 6)  // Supports colors (ANSI or otherwise)
#define CON_CAP_CURSOR_ADDR       (1ULL << 7)  // Can move cursor to absolute position
#define CON_CAP_FRAMEBUFFER_TEXT  (1ULL << 8)  // Renders text onto a pixel framebuffer
#define CON_CAP_UTF8              (1ULL << 9)  // Supports UTF-8 decode/render
#define CON_CAP_SCROLLBACK        (1ULL << 10) // Has a hardware or driver-level scrollback

// Input
#define CON_CAP_INPUT             (1ULL << 11) // Supports reading input

#endif // BHARAT_CONSOLE_CAPS_H
