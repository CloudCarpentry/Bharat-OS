#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "address_table.h"

int main() {
    printf("Running netmgr_addr_remove tests...\n");

    netmgr_addr_table_init();

    net_if_id_t test_if_id = 1;
    net_af_t test_af = NET_AF_INET;
    uint8_t test_addr[4] = {192, 168, 1, 100};
    uint8_t test_prefix = 24;

    // Test 1: Remove an existing address (Happy path)
    assert(netmgr_addr_add(test_if_id, test_af, test_addr, test_prefix) == NETMGR_STATUS_OK);
    assert(netmgr_addr_get(test_if_id, test_af, test_addr, test_prefix) != NULL);
    assert(netmgr_addr_remove(test_if_id, test_af, test_addr, test_prefix) == NETMGR_STATUS_OK);
    assert(netmgr_addr_get(test_if_id, test_af, test_addr, test_prefix) == NULL);

    // Test 2: Remove a non-existent address
    uint8_t wrong_addr[4] = {10, 0, 0, 1};
    assert(netmgr_addr_remove(test_if_id, test_af, wrong_addr, test_prefix) == NETMGR_STATUS_ERR_NOTFOUND);

    // Test 3: Remove with invalid parameters
    assert(netmgr_addr_remove(NET_IF_ID_INVALID, test_af, test_addr, test_prefix) == NETMGR_STATUS_ERR_NOTFOUND);
    assert(netmgr_addr_remove(test_if_id, NET_AF_UNSPEC, test_addr, test_prefix) == NETMGR_STATUS_ERR_NOTFOUND);
    assert(netmgr_addr_remove(test_if_id, test_af, NULL, test_prefix) == NETMGR_STATUS_ERR_NOTFOUND);

    printf("All netmgr_addr_remove tests passed!\n");
    return 0;
}
