#include <bharat/kernel/ds/bh_bitmap.h>
#include <lib/base/string.h>

kstatus_t bh_bitmap_init(bh_bitmap_t *bitmap, void *storage, size_t size_bits) {
    if (!bitmap || !storage) {
        return K_ERR_INVALID_ARG;
    }
    bitmap->bits = (uint8_t *)storage;
    bitmap->size_bits = size_bits;
    memset(storage, 0, (size_bits + 7) / 8);
    return K_OK;
}

kstatus_t bh_bitmap_set(bh_bitmap_t *bitmap, size_t bit) {
    if (bit >= bitmap->size_bits) {
        return K_ERR_INVALID_ARG;
    }
    bitmap->bits[bit / 8] |= (1 << (bit % 8));
    return K_OK;
}

kstatus_t bh_bitmap_clear(bh_bitmap_t *bitmap, size_t bit) {
    if (bit >= bitmap->size_bits) {
        return K_ERR_INVALID_ARG;
    }
    bitmap->bits[bit / 8] &= ~(1 << (bit % 8));
    return K_OK;
}

bool bh_bitmap_test(const bh_bitmap_t *bitmap, size_t bit) {
    if (bit >= bitmap->size_bits) {
        return false;
    }
    return (bitmap->bits[bit / 8] & (1 << (bit % 8))) != 0;
}

kstatus_t bh_bitmap_find_first_clear(const bh_bitmap_t *bitmap, size_t *out_bit) {
    for (size_t i = 0; i < bitmap->size_bits; i++) {
        if (!bh_bitmap_test(bitmap, i)) {
            *out_bit = i;
            return K_OK;
        }
    }
    return K_ERR_NOT_FOUND;
}

kstatus_t bh_bitmap_find_first_set(const bh_bitmap_t *bitmap, size_t *out_bit) {
    for (size_t i = 0; i < bitmap->size_bits; i++) {
        if (bh_bitmap_test(bitmap, i)) {
            *out_bit = i;
            return K_OK;
        }
    }
    return K_ERR_NOT_FOUND;
}

kstatus_t bh_bitmap_set_range(bh_bitmap_t *bitmap, size_t start, size_t count) {
    if (start + count > bitmap->size_bits) {
        return K_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < count; i++) {
        bh_bitmap_set(bitmap, start + i);
    }
    return K_OK;
}

kstatus_t bh_bitmap_clear_range(bh_bitmap_t *bitmap, size_t start, size_t count) {
    if (start + count > bitmap->size_bits) {
        return K_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < count; i++) {
        bh_bitmap_clear(bitmap, start + i);
    }
    return K_OK;
}
