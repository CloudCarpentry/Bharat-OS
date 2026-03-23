#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "interface_table.h"

int main() {
    printf("Running netmgr_iface_set_admin_state tests...\n");

    netmgr_iface_table_init();

    net_if_id_t test_if_id;
    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

    // Setup: Create an interface
    assert(netmgr_iface_create("eth0", mac, 1500, &test_if_id) == NETMGR_STATUS_OK);

    // Test 1: Set admin state to true (Happy path)
    assert(netmgr_iface_set_admin_state(test_if_id, true) == NETMGR_STATUS_OK);

    netmgr_iface_t* iface = netmgr_iface_get(test_if_id);
    assert(iface != NULL);
    assert(iface->admin_up == true);

    // Test 2: Set admin state to false (Happy path)
    assert(netmgr_iface_set_admin_state(test_if_id, false) == NETMGR_STATUS_OK);

    iface = netmgr_iface_get(test_if_id);
    assert(iface != NULL);
    assert(iface->admin_up == false);

    // Test 3: Set admin state on a non-existent interface (Error path)
    net_if_id_t invalid_if_id = 999;
    assert(netmgr_iface_set_admin_state(invalid_if_id, true) == NETMGR_STATUS_ERR_NOTFOUND);

    // Test 4: Set admin state using an explicitly invalid ID (Error path)
    assert(netmgr_iface_set_admin_state(NET_IF_ID_INVALID, true) == NETMGR_STATUS_ERR_NOTFOUND);

    printf("All netmgr_iface_set_admin_state tests passed!\n");
    return 0;
}
