#include <stdint.h>

typedef struct {
    int fd;
    uint32_t vnode_id;
    uint32_t rights;
    uint64_t offset;
} vfs_file_desc_t;

vfs_file_desc_t active_fds[64];

void vfs_fd_init() {
    for(int i=0; i<64; i++) {
        active_fds[i].fd = -1;
    }
}

int vfs_fd_allocate(uint32_t vnode_id, uint32_t rights) {
    for(int i=0; i<64; i++) {
        if (active_fds[i].fd == -1) {
            active_fds[i].fd = i;
            active_fds[i].vnode_id = vnode_id;
            active_fds[i].rights = rights;
            active_fds[i].offset = 0;
            return i;
        }
    }
    return -1;
}
