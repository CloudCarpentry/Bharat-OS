#include <assert.h>
#include <stdio.h>

#include "subsystem_profile.h"

int main(void) {
    bharat_subsystems_init("vm");
    assert(bharat_storage_has(BHARAT_STORAGE_BLOCK_LAYER));
    assert(bharat_storage_has(BHARAT_STORAGE_RAMDISK));
    assert(bharat_network_has(BHARAT_NET_VIRTIO));
    assert(bharat_network_has(BHARAT_NET_FULL_TCPIP_STACK));

    bharat_subsystems_init("mobile");
    assert(bharat_storage_has(BHARAT_STORAGE_EMMC_SD));
    assert(bharat_storage_has(BHARAT_STORAGE_FLASH_MTD));
    assert(bharat_network_has(BHARAT_NET_WIFI));
    assert(bharat_filesystem_has(BHARAT_FS_VFS));
    assert(bharat_filesystem_has(BHARAT_FS_PAGE_CACHE));
    assert(bharat_filesystem_has(BHARAT_FS_WRITEBACK));
    assert(bharat_filesystem_has(BHARAT_FS_TMPFS));
    assert(bharat_filesystem_has(BHARAT_FS_FAT_LIKE));

    bharat_subsystems_init("embedded");
    assert(bharat_filesystem_has(BHARAT_FS_RAMFS));
    assert(bharat_filesystem_has(BHARAT_FS_LITTLEFS));
    assert(!bharat_filesystem_has(BHARAT_FS_JOURNALING));

    bharat_subsystems_init("datacenter");
    assert(bharat_filesystem_has(BHARAT_FS_EXT_LIKE));
    assert(bharat_filesystem_has(BHARAT_FS_JOURNALING));
    assert(bharat_filesystem_has(BHARAT_FS_CRASH_RECOVERY_STRONG));
    assert(bharat_filesystem_has(BHARAT_FS_SCALABLE_WRITEBACK));

    printf("test_subsystem_profile passed\n");
    return 0;
}
