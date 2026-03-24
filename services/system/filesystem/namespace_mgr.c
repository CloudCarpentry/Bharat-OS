#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t ns_id;
    uint32_t allowed_mounts[16];
    uint32_t num_allowed;
} vfs_namespace_t;

vfs_namespace_t active_namespaces[16];
uint32_t num_namespaces = 0;

int vfs_namespace_create() {
    if (num_namespaces >= 16) return -1;
    vfs_namespace_t* ns = &active_namespaces[num_namespaces];
    ns->ns_id = num_namespaces++;
    ns->num_allowed = 0;
    return ns->ns_id;
}
