#include <assert.h>
#include <stdio.h>
#include "core/lib/gfx/region.h"

int main(void) {
    printf("Running test_region...\n");

    bharat_gfx_rect_t r1 = {10, 10, 100, 100};
    bharat_gfx_rect_t r2 = {50, 50, 100, 100};
    bharat_gfx_rect_t res;

    /* Intersection */
    assert(bharat_gfx_rect_intersect(&res, &r1, &r2) == true);
    assert(res.x == 50 && res.y == 50 && res.width == 60 && res.height == 60);

    /* No intersection */
    bharat_gfx_rect_t r3 = {200, 200, 10, 10};
    assert(bharat_gfx_rect_intersect(&res, &r1, &r3) == false);

    /* Union */
    bharat_gfx_rect_union(&res, &r1, &r2);
    assert(res.x == 10 && res.y == 10 && res.width == 140 && res.height == 140);

    /* Contains point */
    assert(bharat_gfx_rect_contains_point(&r1, 15, 15) == true);
    assert(bharat_gfx_rect_contains_point(&r1, 5, 5) == false);

    printf("test_region passed!\n");
    return 0;
}
