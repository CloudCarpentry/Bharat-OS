#include "../../include/mm/vm_object.h"
#include "../../include/slab.h"

void vm_object_retain(vm_object_t *obj) {
    if (obj) {
        __atomic_add_fetch(&obj->refcount, 1, __ATOMIC_SEQ_CST);
    }
}

void vm_object_release(vm_object_t *obj) {
    if (obj) {
        if (__atomic_sub_fetch(&obj->refcount, 1, __ATOMIC_SEQ_CST) == 0) {
            if (obj->ops && obj->ops->release) {
                obj->ops->release(obj);
            }
            kfree(obj);
        }
    }
}
