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

    printf("test_subsystem_profile passed\n");
    return 0;
}
