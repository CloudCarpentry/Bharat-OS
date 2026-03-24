#ifndef BHARAT_UAPI_SYSCALL_NR_H
#define BHARAT_UAPI_SYSCALL_NR_H

#ifdef __cplusplus
extern "C" {
#endif

#define BHARAT_SYSCALL_ABI_VERSION_MAJOR 1U
#define BHARAT_SYSCALL_ABI_VERSION_MINOR 0U
#define BHARAT_SYSCALL_ABI_FROZEN 1U

/* Numbering discipline: 0..255 core kernel ABI, 256..1023 reserved for future kernel ABI growth. */
#define BHARAT_SYSCALL_CORE_MIN 0U
#define BHARAT_SYSCALL_CORE_MAX 255U

typedef enum {
#define SYSCALL_DEF(name, number) name = number,
#include "syscall_table.def"
#undef SYSCALL_DEF
} syscall_id_t;

enum {
    SYSCALL_MAX = SYSCALL_SCHED_SET_AFFINITY
};

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_SYSCALL_NR_H */
