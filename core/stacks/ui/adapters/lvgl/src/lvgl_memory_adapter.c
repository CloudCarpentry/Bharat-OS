/* SPDX-License-Identifier: MIT */
#include <stdlib.h>
#include <string.h>

void *bharat_lvgl_malloc(size_t size) {
    return malloc(size);
}

void bharat_lvgl_free(void *ptr) {
    free(ptr);
}

void *bharat_lvgl_realloc(void *ptr, size_t new_size) {
    return realloc(ptr, new_size);
}
