#include "bharat/console.h"
#include "bharat/ui/fb_console.h"

static int fb_backend_init(console_backend_t* backend) {
    (void)backend;
    return 0;
}

static void fb_backend_write(console_backend_t* backend, const char* str, size_t len) {
    (void)backend;
    for (size_t i = 0; i < len; i++) {
        fb_console_putc(str[i]);
    }
}

static void fb_backend_flush(console_backend_t* backend) {
    (void)backend;
}

static uint64_t fb_backend_query_caps(console_backend_t* backend) {
    (void)backend;
    return CON_CAP_WRITE_POLLING | CON_CAP_FRAMEBUFFER_TEXT | CON_CAP_COLOR | CON_CAP_CURSOR_ADDR;
}

static const console_backend_ops_t fb_ops = {
    .init = fb_backend_init,
    .late_init = NULL,
    .write = fb_backend_write,
    .write_atomic = fb_backend_write,
    .flush = fb_backend_flush,
    .panic_flush = fb_backend_flush,
    .set_mode = NULL,
    .query_caps = fb_backend_query_caps,
    .get_geometry = NULL,
    .scroll = NULL,
    .clear = NULL,
    .set_palette = NULL,
    .poll_input = NULL
};

static console_backend_t g_fb_backend = {
    .type = CONSOLE_TYPE_FRAMEBUFFER,
    .name = "framebuffer",
    .caps = CON_CAP_WRITE_POLLING | CON_CAP_FRAMEBUFFER_TEXT | CON_CAP_COLOR | CON_CAP_CURSOR_ADDR,
    .enabled = true,
    .min_level = CONSOLE_LEVEL_INFO, // Usually display info+, not all debug spam
    .priority = 10,
    .early_ok = false,
    .panic_ok = true, // Framebuffer can be safe if rendered lock-free
    .ops = &fb_ops,
    .priv_data = NULL,
    .next = NULL
};

void console_register_fb_backend(void);
void console_register_fb_backend(void) {
    console_register_backend(&g_fb_backend);
}
