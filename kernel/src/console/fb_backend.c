#include "bharat/console.h"
#include "bharat/ui/fb_console.h"

static int fb_backend_init(void) {
    // Rely on fb_console_init being called separately with a device context
    // or assume it's already initialized
    return 0;
}

static void fb_backend_write(const char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        fb_console_putc(str[i]);
    }
}

static void fb_backend_flush(void) {
    // fb_console_putc renders immediately for now
}

static console_backend_t g_fb_backend = {
    .type = CONSOLE_BACKEND_FRAMEBUFFER,
    .name = "framebuffer",
    .init = fb_backend_init,
    .write = fb_backend_write,
    .flush = fb_backend_flush,
    .next = NULL
};

void console_register_fb_backend(void) {
    console_register_backend(&g_fb_backend);
}
