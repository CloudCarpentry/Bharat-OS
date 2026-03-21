#include <stdio.h>
#include <assert.h>
#include "drivers/can/can_filter.h"
#include "drivers/can/can_frame.h"

void test_exact_id_match() {
    can_filter_t filter = { .id = 0x123, .mask = 0x7FF, .match_extended = false, .extended_only = false };
    can_frame_t frame = { .id = 0x123, .is_extended = false };
    can_frame_t frame_mismatch = { .id = 0x124, .is_extended = false };

    assert(can_filter_match(&filter, &frame) == true);
    assert(can_filter_match(&filter, &frame_mismatch) == false);
}

void test_mask_match() {
    can_filter_t filter = { .id = 0x100, .mask = 0x700, .match_extended = true, .extended_only = false };
    can_frame_t frame_match1 = { .id = 0x123, .is_extended = false };
    can_frame_t frame_match2 = { .id = 0x1FF, .is_extended = true };
    can_frame_t frame_mismatch = { .id = 0x200, .is_extended = false };

    assert(can_filter_match(&filter, &frame_match1) == true);
    assert(can_filter_match(&filter, &frame_match2) == true);
    assert(can_filter_match(&filter, &frame_mismatch) == false);
}

void test_extended_only() {
    can_filter_t filter = { .id = 0x123, .mask = 0x7FF, .match_extended = true, .extended_only = true };
    can_frame_t frame_ext = { .id = 0x123, .is_extended = true };
    can_frame_t frame_std = { .id = 0x123, .is_extended = false };

    assert(can_filter_match(&filter, &frame_ext) == true);
    assert(can_filter_match(&filter, &frame_std) == false);
}

int main() {
    test_exact_id_match();
    test_mask_match();
    test_extended_only();
    printf("test_can_filter_match passed.\n");
    return 0;
}
