#ifndef BHARAT_FREESTANDING_STDINT_H
#define BHARAT_FREESTANDING_STDINT_H

typedef __INT8_TYPE__ int8_t;
typedef __UINT8_TYPE__ uint8_t;
typedef __INT16_TYPE__ int16_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __INT32_TYPE__ int32_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __INT64_TYPE__ int64_t;
typedef __UINT64_TYPE__ uint64_t;

typedef __INTPTR_TYPE__ intptr_t;
typedef __UINTPTR_TYPE__ uintptr_t;
typedef __INTMAX_TYPE__ intmax_t;
typedef __UINTMAX_TYPE__ uintmax_t;
typedef int8_t int_least8_t;
typedef uint8_t uint_least8_t;
typedef int16_t int_least16_t;
typedef uint16_t uint_least16_t;
typedef int32_t int_least32_t;
typedef uint32_t uint_least32_t;
typedef int64_t int_least64_t;
typedef uint64_t uint_least64_t;
typedef __INT_FAST8_TYPE__ int_fast8_t;
typedef __UINT_FAST8_TYPE__ uint_fast8_t;
typedef __INT_FAST16_TYPE__ int_fast16_t;
typedef __UINT_FAST16_TYPE__ uint_fast16_t;
typedef __INT_FAST32_TYPE__ int_fast32_t;
typedef __UINT_FAST32_TYPE__ uint_fast32_t;
typedef __INT_FAST64_TYPE__ int_fast64_t;
typedef __UINT_FAST64_TYPE__ uint_fast64_t;

#define INT8_MIN (-128)
#define INT8_MAX 127
#define UINT8_MAX 255u

#define INT16_MIN (-32768)
#define INT16_MAX 32767
#define UINT16_MAX 65535u

#define INT32_MIN (-2147483647 - 1)
#define INT32_MAX 2147483647
#define UINT32_MAX 4294967295u

#define INT64_MIN (-9223372036854775807ll - 1ll)
#define INT64_MAX 9223372036854775807ll
#define UINT64_MAX 18446744073709551615ull

#define INTPTR_MIN ((intptr_t)(-__INTPTR_MAX__ - 1))
#define INTPTR_MAX ((intptr_t)__INTPTR_MAX__)
#define UINTPTR_MAX ((uintptr_t)__UINTPTR_MAX__)

#define INTMAX_MIN ((intmax_t)(-__INTMAX_MAX__ - 1))
#define INTMAX_MAX ((intmax_t)__INTMAX_MAX__)
#define UINTMAX_MAX ((uintmax_t)__UINTMAX_MAX__)

#define INT8_C(v) v
#define UINT8_C(v) v##u
#define INT16_C(v) v
#define UINT16_C(v) v##u
#define INT32_C(v) v
#define UINT32_C(v) v##u
#define INT64_C(v) v##ll
#define UINT64_C(v) v##ull
#define INTMAX_C(v) v##ll
#define UINTMAX_C(v) v##ull

#endif
