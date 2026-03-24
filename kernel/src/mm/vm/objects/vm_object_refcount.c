#include "../../../../include/mm/vm_object.h"
#include "../../../../include/slab.h"

// Panic function prototype to halt on misuse.
// Depending on architecture or config this could be logging.
extern void kpanic(const char *msg);

void vm_object_retain(vm_object_t *obj) {
    if (obj) {
        // Assert magic is alive
        if (obj->magic != VM_OBJECT_MAGIC_ALIVE) {
            // Using a simple return or panic. For now we just return to avoid hanging tests,
            // or we could use __builtin_trap() which sends SIGILL and fails a test cleanly.
            __builtin_trap();
        }

        __atomic_add_fetch(&obj->refcount, 1, __ATOMIC_SEQ_CST);
    }
}

void vm_object_release(vm_object_t *obj) {
    if (obj) {
        if (obj->magic != VM_OBJECT_MAGIC_ALIVE) {
            // Double release or invalid object!
            __builtin_trap();
        }

        uint32_t old_ref = __atomic_fetch_sub(&obj->refcount, 1, __ATOMIC_SEQ_CST);
        if (old_ref == 0) {
            // Refcount underflow!
            __builtin_trap();
        }

        if (old_ref == 1) {
            // Transition to dead state
            obj->magic = VM_OBJECT_MAGIC_DEAD;

            if (obj->ops && obj->ops->release) {
                obj->ops->release(obj);
            }

            // Poison fields before free
            obj->ops = NULL;
            obj->u.file.backing = NULL;

            kfree(obj);
        }
    }
}
