#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "route_table.h"

static void test_ipv4_basics(void) {
    printf("  Running test_ipv4_basics...\n");
    netmgr_route_table_init();

    net_if_id_t if_id = 1;
    uint8_t ip_any[4] = {0, 0, 0, 0};
    uint8_t mask_any[4] = {0, 0, 0, 0};

    uint8_t ip_a[4] = {10, 0, 0, 0};
    uint8_t mask_8[4] = {255, 0, 0, 0};

    uint8_t ip_b[4] = {10, 20, 0, 0};
    uint8_t mask_16[4] = {255, 255, 0, 0};

    uint8_t ip_c[4] = {10, 20, 30, 0};
    uint8_t mask_24[4] = {255, 255, 255, 0};

    uint8_t ip_d[4] = {10, 20, 30, 40};
    uint8_t mask_32[4] = {255, 255, 255, 255};

    uint8_t gw[4] = {192, 168, 1, 1};

    assert(netmgr_route_add(if_id, NET_AF_INET, ip_any, mask_any, gw, 100) == NETMGR_STATUS_OK);
    assert(netmgr_route_add(if_id, NET_AF_INET, ip_a, mask_8, gw, 80) == NETMGR_STATUS_OK);
    assert(netmgr_route_add(if_id, NET_AF_INET, ip_b, mask_16, gw, 60) == NETMGR_STATUS_OK);
    assert(netmgr_route_add(if_id, NET_AF_INET, ip_c, mask_24, gw, 40) == NETMGR_STATUS_OK);
    assert(netmgr_route_add(if_id, NET_AF_INET, ip_d, mask_32, gw, 20) == NETMGR_STATUS_OK);

    uint8_t target1[4] = {8, 8, 8, 8};
    netmgr_route_t *r = netmgr_route_lookup(NET_AF_INET, target1);
    assert(r != NULL);
    assert(r->prefix_len == 0);

    uint8_t target2[4] = {10, 99, 1, 1};
    r = netmgr_route_lookup(NET_AF_INET, target2);
    assert(r != NULL);
    assert(r->prefix_len == 8);

    uint8_t target3[4] = {10, 20, 8, 8};
    r = netmgr_route_lookup(NET_AF_INET, target3);
    assert(r != NULL);
    assert(r->prefix_len == 16);

    uint8_t target4[4] = {10, 20, 30, 99};
    r = netmgr_route_lookup(NET_AF_INET, target4);
    assert(r != NULL);
    assert(r->prefix_len == 24);

    uint8_t target5[4] = {10, 20, 30, 40};
    r = netmgr_route_lookup(NET_AF_INET, target5);
    assert(r != NULL);
    assert(r->prefix_len == 32);
}

static void test_canonicalization(void) {
    printf("  Running test_canonicalization...\n");
    netmgr_route_table_init();

    net_if_id_t if_id = 2;
    uint8_t ip_raw[4] = {10, 20, 30, 77};
    uint8_t mask_24[4] = {255, 255, 255, 0};
    uint8_t gw[4] = {10, 20, 30, 1};

    assert(netmgr_route_add(if_id, NET_AF_INET, ip_raw, mask_24, gw, 10) == NETMGR_STATUS_OK);

    uint8_t target[4] = {10, 20, 30, 99};
    netmgr_route_t *r = netmgr_route_lookup(NET_AF_INET, target);
    assert(r != NULL);
    assert(r->dest[0] == 10 && r->dest[1] == 20 && r->dest[2] == 30 && r->dest[3] == 0);

    assert(netmgr_route_add(if_id, NET_AF_INET, target, mask_24, NULL, 5) == NETMGR_STATUS_OK);

    r = netmgr_route_lookup(NET_AF_INET, target);
    assert(r != NULL);
    assert(r->metric == 5);
    for (int i = 0; i < 16; i++) {
        assert(r->gateway[i] == 0);
    }

    uint8_t ip_other[4] = {10, 20, 30, 53};
    assert(netmgr_route_remove(NET_AF_INET, ip_other, mask_24) == NETMGR_STATUS_OK);

    assert(netmgr_route_lookup(NET_AF_INET, target) == NULL);
}

static void test_ipv6_basics(void) {
    printf("  Running test_ipv6_basics...\n");
    netmgr_route_table_init();

    net_if_id_t if_id = 3;
    uint8_t ip6_any[16] = {0};
    uint8_t mask6_any[16] = {0};

    uint8_t ip6_a[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t mask6_32[16] = {0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    uint8_t ip6_b[16] = {0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0x56, 0x78, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t mask6_64[16] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0};

    assert(netmgr_route_add(if_id, NET_AF_INET6, ip6_any, mask6_any, NULL, 50) == NETMGR_STATUS_OK);
    assert(netmgr_route_add(if_id, NET_AF_INET6, ip6_a, mask6_32, NULL, 30) == NETMGR_STATUS_OK);
    assert(netmgr_route_add(if_id, NET_AF_INET6, ip6_b, mask6_64, NULL, 10) == NETMGR_STATUS_OK);

    uint8_t target_any[16] = {0x24, 0x00, 0x01, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    netmgr_route_t *r = netmgr_route_lookup(NET_AF_INET6, target_any);
    assert(r != NULL);
    assert(r->prefix_len == 0);

    uint8_t target_32[16] = {0x20, 0x01, 0x0d, 0xb8, 0x99, 0x99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    r = netmgr_route_lookup(NET_AF_INET6, target_32);
    assert(r != NULL);
    assert(r->prefix_len == 32);

    uint8_t target_64[16] = {0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0x56, 0x78, 0xaa, 0xbb, 0, 0, 0, 0, 0, 0};
    r = netmgr_route_lookup(NET_AF_INET6, target_64);
    assert(r != NULL);
    assert(r->prefix_len == 64);
}

static void test_invalid_masks(void) {
    printf("  Running test_invalid_masks...\n");
    netmgr_route_table_init();

    net_if_id_t if_id = 4;
    uint8_t dest[16] = {10, 0, 0, 0};

    uint8_t invalid_m1[4] = {255, 0, 255, 0};
    uint8_t invalid_m2[4] = {255, 255, 127, 255};
    uint8_t invalid_m3[4] = {0, 255, 255, 0};

    assert(netmgr_route_add(if_id, NET_AF_INET, dest, invalid_m1, NULL, 10) == NETMGR_STATUS_ERR_INVAL);
    assert(netmgr_route_add(if_id, NET_AF_INET, dest, invalid_m2, NULL, 10) == NETMGR_STATUS_ERR_INVAL);
    assert(netmgr_route_add(if_id, NET_AF_INET, dest, invalid_m3, NULL, 10) == NETMGR_STATUS_ERR_INVAL);

    uint8_t invalid_m6_1[16] = {0xff, 0x00, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t invalid_m6_2[16] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0, 0, 0, 0, 0, 0, 0};

    assert(netmgr_route_add(if_id, NET_AF_INET6, dest, invalid_m6_1, NULL, 10) == NETMGR_STATUS_ERR_INVAL);
    assert(netmgr_route_add(if_id, NET_AF_INET6, dest, invalid_m6_2, NULL, 10) == NETMGR_STATUS_ERR_INVAL);
}

static void test_invalid_address_families(void) {
    printf("  Running test_invalid_address_families...\n");
    netmgr_route_table_init();

    net_if_id_t if_id = 5;
    uint8_t dest[16] = {10, 0, 0, 0};
    uint8_t mask[16] = {255, 255, 255, 0};

    assert(netmgr_route_add(if_id, NET_AF_UNSPEC, dest, mask, NULL, 10) == NETMGR_STATUS_ERR_INVAL);
    assert(netmgr_route_add(if_id, NET_AF_PACKET, dest, mask, NULL, 10) == NETMGR_STATUS_ERR_INVAL);
    assert(netmgr_route_add(if_id, (net_af_t)99, dest, mask, NULL, 10) == NETMGR_STATUS_ERR_INVAL);

    assert(netmgr_route_lookup(NET_AF_UNSPEC, dest) == NULL);
    assert(netmgr_route_lookup(NET_AF_PACKET, dest) == NULL);
    assert(netmgr_route_lookup((net_af_t)99, dest) == NULL);

    assert(netmgr_route_remove(NET_AF_UNSPEC, dest, mask) == NETMGR_STATUS_ERR_INVAL);
    assert(netmgr_route_remove(NET_AF_PACKET, dest, mask) == NETMGR_STATUS_ERR_INVAL);
    assert(netmgr_route_remove((net_af_t)99, dest, mask) == NETMGR_STATUS_ERR_INVAL);
}

static void test_capacity_limits(void) {
    printf("  Running test_capacity_limits...\n");
    netmgr_route_table_init();

    net_if_id_t if_id = 6;
    uint8_t mask[4] = {255, 255, 255, 255};

    for (int i = 0; i < NETMGR_MAX_ROUTES; i++) {
        uint8_t dest[4] = {10, 0, (uint8_t)(i >> 8), (uint8_t)i};
        assert(netmgr_route_add(if_id, NET_AF_INET, dest, mask, NULL, 10) == NETMGR_STATUS_OK);
    }

    uint8_t dest_65[4] = {20, 0, 0, 1};
    assert(netmgr_route_add(if_id, NET_AF_INET, dest_65, mask, NULL, 10) == NETMGR_STATUS_ERR_NOSPACE);

    uint8_t dest_existing[4] = {10, 0, 0, 0};
    assert(netmgr_route_add(if_id, NET_AF_INET, dest_existing, mask, NULL, 20) == NETMGR_STATUS_OK);

    netmgr_route_t *r = netmgr_route_lookup(NET_AF_INET, dest_existing);
    assert(r != NULL);
    assert(r->metric == 20);
}

static void test_slot_reuse_hygiene(void) {
    printf("  Running test_slot_reuse_hygiene...\n");
    netmgr_route_table_init();

    net_if_id_t if_id = 7;

    uint8_t ip6_dest[16] = {0x20, 0x01, 0x0d, 0xb8, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
    uint8_t ip6_mask[16] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t ip6_gw[16] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

    assert(netmgr_route_add(if_id, NET_AF_INET6, ip6_dest, ip6_mask, ip6_gw, 10) == NETMGR_STATUS_OK);

    netmgr_route_t *r = netmgr_route_lookup(NET_AF_INET6, ip6_dest);
    assert(r != NULL);
    assert(r->af == NET_AF_INET6);

    assert(netmgr_route_remove(NET_AF_INET6, ip6_dest, ip6_mask) == NETMGR_STATUS_OK);

    uint8_t ip4_dest[4] = {192, 168, 1, 0};
    uint8_t ip4_mask[4] = {255, 255, 255, 0};
    uint8_t ip4_gw[4] = {192, 168, 1, 254};

    assert(netmgr_route_add(if_id, NET_AF_INET, ip4_dest, ip4_mask, ip4_gw, 20) == NETMGR_STATUS_OK);

    r = netmgr_route_lookup(NET_AF_INET, ip4_dest);
    assert(r != NULL);
    assert(r->af == NET_AF_INET);
    assert(r->prefix_len == 24);

    for (int i = 4; i < 16; i++) {
        assert(r->dest[i] == 0);
        assert(r->mask[i] == 0);
        assert(r->gateway[i] == 0);
    }
}

int main(void) {
    printf("Starting NET-P2-001 Route Table Hardening tests...\n");

    test_ipv4_basics();
    test_canonicalization();
    test_ipv6_basics();
    test_invalid_masks();
    test_invalid_address_families();
    test_capacity_limits();
    test_slot_reuse_hygiene();

    printf("All NET-P2-001 tests passed successfully!\n");
    return 0;
}
