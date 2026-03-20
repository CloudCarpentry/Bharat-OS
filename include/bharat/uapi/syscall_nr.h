#ifndef BHARAT_UAPI_SYSCALL_NR_H
#define BHARAT_UAPI_SYSCALL_NR_H

#ifdef __cplusplus
extern "C" {
#endif

#define BHARAT_SYSCALL_ABI_VERSION_MAJOR 1U
#define BHARAT_SYSCALL_ABI_VERSION_MINOR 0U

typedef enum {
#define SYSCALL_DEF(name, number) name = number,
#include "syscall_table.def"
#undef SYSCALL_DEF
} syscall_id_t;

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_SYSCALL_NR_H */
