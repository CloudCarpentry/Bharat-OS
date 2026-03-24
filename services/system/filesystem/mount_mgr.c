#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t mount_id;
    char path_prefix[256];
    uint32_t capabilities;
} vfs_mount_t;

vfs_mount_t active_mounts[16];
uint32_t num_mounts = 0;

int vfs_mount_add(const char* prefix, uint32_t capabilities) {
    if (num_mounts >= 16) return -1;
    vfs_mount_t* m = &active_mounts[num_mounts];
    m->mount_id = num_mounts++;
    m->capabilities = capabilities;
    // basic copy
    for(int i=0; i<255 && prefix[i] != '\0'; i++) {
        m->path_prefix[i] = prefix[i];
        m->path_prefix[i+1] = '\0';
    }
    return m->mount_id;
}
