#ifndef BHARAT_USER_IMAGE_LOADER_H
#define BHARAT_USER_IMAGE_LOADER_H

#include <stdint.h>
#include <stddef.h>
#include "kernel/status.h"
#include "sched/sched.h"
#include "mm/aspace.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const void *bytes;
    size_t size;
    uint64_t image_id;
    uint64_t flags;
} bh_user_image_t;

typedef struct {
    uintptr_t entry_point;
    uintptr_t user_stack_top;
    uintptr_t startup_va;
    address_space_t *aspace;
} bh_user_image_result_t;

/**
 * Loads an executable image into the provided process.
 *
 * @param process The process to load the image into. Must have a valid address space.
 * @param aspace The address space to use. Usually process->addr_space.
 * @param image The image (e.g. ELF) to load.
 * @param out Pointer to store the result, including entry point and stack top.
 * @return K_OK on success, or a relevant error code.
 */
kstatus_t bh_user_image_load(
    bh_process_t *process,
    address_space_t *aspace,
    const bh_user_image_t *image,
    bh_user_image_result_t *out);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_USER_IMAGE_LOADER_H
