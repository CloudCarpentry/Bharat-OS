#ifndef BHARAT_LIB_GFX_REGION_H
#define BHARAT_LIB_GFX_REGION_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Basic rectangle structure for graphics.
 */
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} bharat_gfx_rect_t;

/**
 * Returns true if the rectangle has non-zero area and positive dimensions.
 */
bool bharat_gfx_rect_is_valid(const bharat_gfx_rect_t *rect);

/**
 * Returns true if the point (x, y) is inside the rectangle.
 */
bool bharat_gfx_rect_contains_point(const bharat_gfx_rect_t *rect, int32_t x, int32_t y);

/**
 * Calculates the intersection of two rectangles.
 * Returns true if they intersect, and fills 'dest' with the intersection.
 */
bool bharat_gfx_rect_intersect(bharat_gfx_rect_t *dest, const bharat_gfx_rect_t *a, const bharat_gfx_rect_t *b);

/**
 * Calculates the bounding box that contains both rectangles.
 */
void bharat_gfx_rect_union(bharat_gfx_rect_t *dest, const bharat_gfx_rect_t *a, const bharat_gfx_rect_t *b);

#endif /* BHARAT_LIB_GFX_REGION_H */
