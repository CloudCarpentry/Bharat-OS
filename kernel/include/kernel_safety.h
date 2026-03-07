#ifndef BHARAT_KERNEL_SAFETY_H
#define BHARAT_KERNEL_SAFETY_H

#include <stddef.h>
#include <stdint.h>

#define BHARAT_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define BHARAT_IS_ALIGNED(value, alignment) \
    (((alignment) != 0U) && (((value) & ((alignment) - 1U)) == 0U))

#define BHARAT_BOUNDS_CHECK(index, count) ((size_t)(index) < (size_t)(count))

#define BHARAT_PTR_NON_NULL(ptr) ((ptr) != NULL)

#define BHARAT_RANGE_VALID(value, minv, maxv) \
    ((uint64_t)(value) >= (uint64_t)(minv) && (uint64_t)(value) <= (uint64_t)(maxv))

#endif // BHARAT_KERNEL_SAFETY_H
