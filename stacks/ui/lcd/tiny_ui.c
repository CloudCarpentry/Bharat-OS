#include "bharat/ui/tiny_ui.h"

static uint32_t ui_min_u32(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

static void put_px_xrgb8888(const bharat_tiny_fb_t *fb, uint32_t x, uint32_t y, uint32_t color) {
    uint8_t *row = (uint8_t *)fb->pixels + ((size_t)y * fb->stride_bytes);
    uint32_t *pixel = (uint32_t *)row + x;
    *pixel = color;
}

static void put_px_mono8(const bharat_tiny_fb_t *fb, uint32_t x, uint32_t y, uint32_t color) {
    uint8_t *row = (uint8_t *)fb->pixels + ((size_t)y * fb->stride_bytes);
    row[x] = (uint8_t)((color & 0x00FFFFFFu) ? 0xFFu : 0x00u);
}

static void draw_rect(const bharat_tiny_fb_t *fb,
                      uint32_t x,
                      uint32_t y,
                      uint32_t width,
                      uint32_t height,
                      uint32_t color) {
    if (!fb || !fb->pixels || x >= fb->width_px || y >= fb->height_px || width == 0 || height == 0) {
        return;
    }

    uint32_t x_end = ui_min_u32(x + width, fb->width_px);
    uint32_t y_end = ui_min_u32(y + height, fb->height_px);

    for (uint32_t row = y; row < y_end; ++row) {
        for (uint32_t col = x; col < x_end; ++col) {
            if (fb->pixel_format == BHARAT_UI_PIXEL_FMT_XRGB8888) {
                put_px_xrgb8888(fb, col, row, color);
            } else {
                put_px_mono8(fb, col, row, color);
            }
        }
    }
}

void bharat_tiny_ui_init(bharat_tiny_ui_state_t *state, bool safe_mode) {
    if (!state) {
        return;
    }

    state->page = BHARAT_UI_PAGE_SPLASH;
    state->progress_percent = 0;
    state->safe_mode = safe_mode;
}

void bharat_tiny_ui_apply_input(bharat_tiny_ui_state_t *state, bharat_ui_input_action_t action) {
    if (!state) {
        return;
    }

    switch (action) {
        case BHARAT_UI_INPUT_NEXT:
            state->page = (bharat_ui_page_id_t)((state->page + 1) % BHARAT_UI_PAGE_MAX);
            break;
        case BHARAT_UI_INPUT_PREV:
            if (state->page == BHARAT_UI_PAGE_SPLASH) {
                state->page = BHARAT_UI_PAGE_RECOVERY;
            } else {
                state->page = (bharat_ui_page_id_t)(state->page - 1);
            }
            break;
        case BHARAT_UI_INPUT_SELECT:
            if (state->page == BHARAT_UI_PAGE_SPLASH && state->progress_percent < 100u) {
                state->progress_percent = (uint8_t)ui_min_u32((uint32_t)state->progress_percent + 10u, 100u);
            }
            break;
        case BHARAT_UI_INPUT_BACK:
            state->page = BHARAT_UI_PAGE_SPLASH;
            break;
        case BHARAT_UI_INPUT_NONE:
        default:
            break;
    }
}

void bharat_tiny_ui_render(const bharat_tiny_fb_t *fb, const bharat_tiny_ui_state_t *state) {
    if (!fb || !state || !fb->pixels || fb->width_px == 0 || fb->height_px == 0) {
        return;
    }

    uint32_t background = state->safe_mode ? 0xFF3A1E00u : 0xFF1D1D1Du;
    draw_rect(fb, 0, 0, fb->width_px, fb->height_px, background);

    uint32_t header_h = fb->height_px / 6u;
    uint32_t body_y = header_h;
    uint32_t body_h = fb->height_px - header_h;

    uint32_t header_color = 0xFF004A99u;
    if (state->page == BHARAT_UI_PAGE_DIAGNOSTICS) {
        header_color = 0xFF1E7A34u;
    } else if (state->page == BHARAT_UI_PAGE_RECOVERY) {
        header_color = 0xFFA04000u;
    }
    draw_rect(fb, 0, 0, fb->width_px, header_h, header_color);

    draw_rect(fb, 0, body_y, fb->width_px, body_h, 0xFF222222u);

    uint32_t margin = fb->width_px / 12u;
    uint32_t bar_y = (fb->height_px > 60u) ? (fb->height_px - 40u) : (fb->height_px - 10u);
    uint32_t bar_w = fb->width_px - (2u * margin);
    uint32_t bar_h = fb->height_px / 18u;
    if (bar_h < 4u) {
        bar_h = 4u;
    }

    draw_rect(fb, margin, bar_y, bar_w, bar_h, 0xFF4D4D4Du);

    uint32_t fill_w = (uint32_t)(((uint64_t)bar_w * state->progress_percent) / 100u);
    uint32_t fill_color = state->safe_mode ? 0xFFFFC247u : 0xFF00D16Fu;
    draw_rect(fb, margin, bar_y, fill_w, bar_h, fill_color);
}
