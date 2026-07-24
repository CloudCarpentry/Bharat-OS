#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define BH_STORAGE_BACKEND_DISABLED 0
#define BH_STORAGE_BACKEND_PRODUCTION 4

typedef struct {
    const char* name;
    int maturity;
    int (*mount)(uint32_t device_id, void** out_fs_handle);
    int (*unmount)(void* fs_handle);
    int (*lookup)(void* fs_handle, void* dir, const char* name, void** out_vnode);
    int (*read)(void* fs_handle, void* vnode, uint64_t offset, void* buf, size_t size);
    int (*write)(void* fs_handle, void* vnode, uint64_t offset, const void* buf, size_t size);
} fs_adapter_t;

int fs_adapter_register(fs_adapter_t* adapter);
const fs_adapter_t* fs_adapter_get(const char* name);

static int mock_mount(uint32_t device_id, void** out_fs_handle) { return 0; }

void test_adapter_maturity(void) {
    fs_adapter_t production_fs = {
        .name = "prod_fs",
        .maturity = BH_STORAGE_BACKEND_PRODUCTION,
        .mount = mock_mount
    };

    fs_adapter_t disabled_fs = {
        .name = "disabled_fs",
        .maturity = BH_STORAGE_BACKEND_DISABLED,
        .mount = mock_mount
    };

    printf("Testing production FS registration...\n");
    assert(fs_adapter_register(&production_fs) == 0);
    printf("Testing disabled FS registration rejection...\n");
    assert(fs_adapter_register(&disabled_fs) == -2);

    assert(fs_adapter_get("prod_fs") == &production_fs);
    assert(fs_adapter_get("disabled_fs") == NULL);
}

int main() {
    printf("Starting backend registry tests...\n");
    test_adapter_maturity();
    printf("Backend registry tests passed!\n");
    return 0;
}
