#ifndef BHARAT_UI_TINY_UI_H
#define BHARAT_UI_TINY_UI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BHARAT_UI_PIXEL_FMT_MONO8 = 0,
    BHARAT_UI_PIXEL_FMT_XRGB8888 = 1,
} bharat_ui_pixel_format_t;

typedef enum {
    BHARAT_UI_PAGE_SPLASH = 0,
    BHARAT_UI_PAGE_DIAGNOSTICS = 1,
    BHARAT_UI_PAGE_RECOVERY = 2,
    BHARAT_UI_PAGE_MAX
} bharat_ui_page_id_t;

typedef enum {
    BHARAT_UI_INPUT_NONE = 0,
    BHARAT_UI_INPUT_NEXT = 1,
    BHARAT_UI_INPUT_PREV = 2,
    BHARAT_UI_INPUT_SELECT = 3,
    BHARAT_UI_INPUT_BACK = 4,
} bharat_ui_input_action_t;

typedef struct {
    uint32_t width_px;
    uint32_t height_px;
    uint32_t stride_bytes;
    bharat_ui_pixel_format_t pixel_format;
    void *pixels;
} bharat_tiny_fb_t;

typedef struct {
    bharat_ui_page_id_t page;
    uint8_t progress_percent;
    bool safe_mode;
} bharat_tiny_ui_state_t;

void bharat_tiny_ui_init(bharat_tiny_ui_state_t *state, bool safe_mode);
void bharat_tiny_ui_apply_input(bharat_tiny_ui_state_t *state, bharat_ui_input_action_t action);
void bharat_tiny_ui_render(const bharat_tiny_fb_t *fb, const bharat_tiny_ui_state_t *state);

#ifdef __cplusplus
}
#endif

#endif
