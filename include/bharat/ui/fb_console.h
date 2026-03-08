#ifndef BHARAT_FB_CONSOLE_H
#define BHARAT_FB_CONSOLE_H

#include "bharat/display/display.h"

void fb_console_init(bharat_display_device_t *dev);
void fb_console_putc(char c);
void fb_console_puts(const char *str);
void fb_console_clear(void);

#endif // BHARAT_FB_CONSOLE_H
