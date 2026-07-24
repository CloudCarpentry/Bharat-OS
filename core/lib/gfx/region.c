#include "region.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

bool bharat_gfx_rect_is_valid(const bharat_gfx_rect_t *rect) {
    return rect && rect->width > 0 && rect->height > 0;
}

bool bharat_gfx_rect_contains_point(const bharat_gfx_rect_t *rect, int32_t x, int32_t y) {
    if (!bharat_gfx_rect_is_valid(rect)) return false;
    return (x >= rect->x && x < rect->x + rect->width &&
            y >= rect->y && y < rect->y + rect->height);
}

bool bharat_gfx_rect_intersect(bharat_gfx_rect_t *dest, const bharat_gfx_rect_t *a, const bharat_gfx_rect_t *b) {
    if (!bharat_gfx_rect_is_valid(a) || !bharat_gfx_rect_is_valid(b)) return false;

    int32_t x1 = MAX(a->x, b->x);
    int32_t y1 = MAX(a->y, b->y);
    int32_t x2 = MIN(a->x + a->width, b->x + b->width);
    int32_t y2 = MIN(a->y + a->height, b->y + b->height);

    if (x2 <= x1 || y2 <= y1) return false;

    if (dest) {
        dest->x = x1;
        dest->y = y1;
        dest->width = x2 - x1;
        dest->height = y2 - y1;
    }
    return true;
}

void bharat_gfx_rect_union(bharat_gfx_rect_t *dest, const bharat_gfx_rect_t *a, const bharat_gfx_rect_t *b) {
    if (!bharat_gfx_rect_is_valid(a)) {
        if (dest && b) *dest = *b;
        return;
    }
    if (!bharat_gfx_rect_is_valid(b)) {
        if (dest && a) *dest = *a;
        return;
    }

    int32_t x1 = MIN(a->x, b->x);
    int32_t y1 = MIN(a->y, b->y);
    int32_t x2 = MAX(a->x + a->width, b->x + b->width);
    int32_t y2 = MAX(a->y + a->height, b->y + b->height);

    if (dest) {
        dest->x = x1;
        dest->y = y1;
        dest->width = x2 - x1;
        dest->height = y2 - y1;
    }
}
