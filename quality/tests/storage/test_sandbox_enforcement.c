#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define BHARAT_FORMAL_VERIF_H
#define BHARAT_VFS_H
#define BHARAT_FS_NAMESPACE_H

typedef struct {
    uint32_t capability_id;
    uint64_t rights_mask; // Use 64-bit for compatibility with production headers
    uint32_t target_object_id;
} capability_t;

typedef struct vfs_node {
    uint32_t object_id;
} vfs_node_t;

typedef struct vfs_mount {
    char target_path[256];
    size_t target_path_len;
    vfs_node_t* root_node;
} vfs_mount_t;

typedef struct vfs_namespace {
    capability_t sandbox_cap;
    vfs_mount_t* mounts;
    uint32_t mount_count;
    vfs_node_t* root_dir;
    vfs_node_t* cwd;
    struct vfs_namespace* parent;
} vfs_namespace_t;

#define VFS_NAMESPACE_OBJECT_ID 0xFFFFFFFF

int vfs_path_prefix_match(const char *path, const char *prefix) {
    size_t i = 0;
    if (!path || !prefix) return 0;
    while (prefix[i] != '\0' && path[i] != '\0') {
        if (prefix[i] != path[i]) return 0;
        i++;
    }
    if (prefix[i] != '\0') return 0;
    return (path[i] == '\0' || path[i] == '/');
}

// Prototypes from namespace_mgr.c
int vfs_namespace_create(vfs_namespace_t* parent, vfs_namespace_t** out_namespace, capability_t* cap);
int vfs_validate_path_rights(const char* path, uint32_t required_rights, capability_t* cap);

void test_namespace_traversal_rejection(void) {
    capability_t cap = { .capability_id = 1, .rights_mask = 0xFFFFFFFFFFFFFFFFULL }; // All rights

    printf("Testing traversal rejection...\n");
    // Normal paths
    int ret = vfs_validate_path_rights("/etc/config", 1, &cap);
    if (ret != 0) printf("Failed normal path: %d\n", ret);
    assert(ret == 0);

    ret = vfs_validate_path_rights("/home/user/.ssh", 1, &cap);
    if (ret != 0) printf("Failed normal path 2: %d\n", ret);
    assert(ret == 0);

    // Traversal attempts
    assert(vfs_validate_path_rights("/etc/../etc/shadow", 1, &cap) == -3);
    assert(vfs_validate_path_rights("../breakout", 1, &cap) == -3);
    assert(vfs_validate_path_rights("/data/..", 1, &cap) == -3);
}

void test_namespace_deny_by_default(void) {
    printf("Testing deny-by-default...\n");
    capability_t root_cap = { .capability_id = 1, .rights_mask = 0xFFFF };
    vfs_namespace_t root_ns = { .sandbox_cap = root_cap };

    vfs_namespace_t* sub_ns = NULL;
    capability_t sub_cap = { .capability_id = 2, .rights_mask = 0x0001 }; // Read only

    assert(vfs_namespace_create(&root_ns, &sub_ns, &sub_cap) == 0);
    assert(sub_ns->sandbox_cap.rights_mask == 0x0001);

    vfs_namespace_t* invalid_ns = NULL;
    capability_t invalid_cap = { .capability_id = 3, .rights_mask = 0xFFFF }; // Trying to escalate
    assert(vfs_namespace_create(sub_ns, &invalid_ns, &invalid_cap) == -2);

    free(sub_ns);
}

int main() {
    printf("Starting tests...\n");
    test_namespace_traversal_rejection();
    test_namespace_deny_by_default();
    printf("Tests passed!\n");
    return 0;
}
