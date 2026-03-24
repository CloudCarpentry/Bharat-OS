#ifndef BHARAT_MM_VM_OBJECT_H
#define BHARAT_MM_VM_OBJECT_H

#include <stddef.h>
#include <stdint.h>

// Forward declare phys_addr_t or include mm.h early
#ifndef BHARAT_MM_H
typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VM_OBJECT_ANON = 0,
    VM_OBJECT_SHARED,
    VM_OBJECT_FILE,
    VM_OBJECT_DEVICE,
    VM_OBJECT_DMA,
} vm_object_kind_t;

typedef enum {
    VM_INHERIT_NONE = 0,      // do not copy into clone
    VM_INHERIT_SHARE,         // clone keeps same backing object
    VM_INHERIT_COPY_META,     // clone copies metadata, still same object for now
} vm_inherit_t;

struct vm_region; // Forward declaration

typedef struct vm_object vm_object_t;

typedef struct {
    uint64_t page_offset;
    uint64_t fault_addr;
    uint32_t access;
    uint32_t fault_flags;
} vm_fault_ctx_t;

// Fault resolution flags (returned by object backend)
#define VM_FAULT_HANDLED    0
#define VM_FAULT_SIGSEGV   -1
#define VM_FAULT_OOM       -2
#define VM_FAULT_SIGBUS    -3
#define VM_FAULT_ENOSYS    -4

typedef struct vm_object_ops {
    int (*fault)(struct vm_object *obj,
                 struct vm_region *region,
                 uintptr_t fault_addr,
                 uint32_t access,
                 phys_addr_t *out_page,
                 uint32_t *out_page_flags);

    void (*release)(struct vm_object *obj);
} vm_object_ops_t;

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "../../include/spinlock.h"
// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "../../include/mm.h"
struct vm_object {
    vm_object_kind_t kind;
    uint32_t flags;
    size_t size;

    uint32_t magic; // Debug cookie for lifecycle validation
    volatile uint32_t refcount;

    const vm_object_ops_t *ops;

    union {
        struct {
            uint32_t zero_fill;
        } anon;

        struct {
            uint32_t shared_id;
        } shared;

        struct {
            void *backing;          // vnode/file/inode/opaque handle
            uint64_t file_offset;
        } file;

        struct {
            phys_addr_t phys_base;
            uint32_t cache_flags;
        } device;

        struct {
            phys_addr_t phys_base;
            uint32_t dma_flags;
            uint32_t numa_node;
        } dma;
    } u;
};

vm_object_t *vm_object_create_anon(size_t size, uint32_t flags);
vm_object_t *vm_object_create_shared(size_t size, uint32_t flags);
vm_object_t *vm_object_create_file(void *backing, uint64_t file_offset, size_t size, uint32_t flags);
vm_object_t *vm_object_create_device(phys_addr_t phys_base, size_t size, uint32_t cache_flags, uint32_t flags);
vm_object_t *vm_object_create_dma(phys_addr_t phys_base, size_t size, uint32_t dma_flags, uint32_t numa_node, uint32_t flags);

void vm_object_retain(vm_object_t *obj);
void vm_object_release(vm_object_t *obj);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_VM_OBJECT_H
