#ifndef BHARATLIBC_BSYS_BACKEND_H
#define BHARATLIBC_BSYS_BACKEND_H

#include <standard/stdint.h>
#include <bharat/bsys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t abi_version;
    uint32_t structure_size;

    int32_t (*write)(
        uint32_t handle,
        const void *buffer,
        uint32_t length,
        uint32_t *written);

    int32_t (*read)(
        uint32_t handle,
        void *buffer,
        uint32_t capacity,
        uint32_t *received);

    int32_t (*close)(
        uint32_t handle);

    int32_t (*clock_gettime)(
        uint32_t clock_id,
        bh_bsys_timespec_t *out_time);

    int32_t (*sleep_until)(
        uint64_t deadline_ticks);

    int32_t (*heap_region)(
        uintptr_t *out_base,
        uint32_t *out_size);

    void (*process_exit)(
        int32_t status);
} bh_bsys_backend_ops_t;

/* Backend registration and query */
void bh_bsys_register_backend(const bh_bsys_backend_ops_t *ops);
const bh_bsys_backend_ops_t *bh_bsys_get_backend(void);

#ifdef __cplusplus
}
#endif

#endif /* BHARATLIBC_BSYS_BACKEND_H */
