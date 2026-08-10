/* SPDX-License-Identifier: MIT */
#ifndef LV_BHARAT_INTTYPES_H
#define LV_BHARAT_INTTYPES_H

/*
 * The bare-metal targets do not provide a hosted <inttypes.h>. Clang exposes
 * the exact format spellings for each target data model through these macros.
 */
#if !defined(__INT32_FMTd__) || !defined(__INT64_FMTd__) ||                  \
    !defined(__UINT32_FMTu__) || !defined(__UINT32_FMTx__) ||               \
    !defined(__UINT32_FMTX__) || !defined(__UINT64_FMTu__) ||               \
    !defined(__UINT64_FMTx__) || !defined(__UINT64_FMTX__)
#error "Compiler does not expose fixed-width integer format macros"
#endif

#define PRId32 __INT32_FMTd__
#define PRId64 __INT64_FMTd__
#define PRIu32 __UINT32_FMTu__
#define PRIu64 __UINT64_FMTu__
#define PRIx32 __UINT32_FMTx__
#define PRIx64 __UINT64_FMTx__
#define PRIX32 __UINT32_FMTX__
#define PRIX64 __UINT64_FMTX__

#endif /* LV_BHARAT_INTTYPES_H */
