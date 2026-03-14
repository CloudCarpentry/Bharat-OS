#ifndef BHARAT_MM_VM_OBJECT_H
#define BHARAT_MM_VM_OBJECT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VM_OBJECT_ANON = 0,
    VM_OBJECT_SHARED_ANON,
    VM_OBJECT_FILE,
    VM_OBJECT_DEVICE,
    VM_OBJECT_DMA,
} vm_object_kind_t;

typedef struct vm_object vm_object_t;

typedef struct {
    uint64_t page_offset;
    uint64_t fault_addr;
    uint32_t access;
    uint32_t fault_flags;
} vm_fault_ctx_t;

typedef struct {
    int (*fault)(vm_object_t *obj, const vm_fault_ctx_t *ctx, uint64_t *out_phys_page);
    int (*writeback)(vm_object_t *obj, uint64_t page_offset);
    void (*release)(vm_object_t *obj);
} vm_object_ops_t;

struct vm_object {
    vm_object_kind_t kind;
    uint64_t size_bytes;
    uint32_t object_flags;
    uint32_t refcount;
    const vm_object_ops_t *ops;
    void *backend;
};

int vm_object_ref(vm_object_t *obj);
int vm_object_unref(vm_object_t *obj);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_VM_OBJECT_H
