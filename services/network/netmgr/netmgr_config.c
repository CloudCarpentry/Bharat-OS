#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char name[16];
    bool use_dhcp;
    uint32_t static_ip;
    uint32_t static_mask;
    uint32_t static_gw;
} iface_config_t;

typedef struct {
    char hostname[32];
    uint32_t dns_server;
    iface_config_t ifaces[4];
} netmgr_config_t;

static netmgr_config_t current_config;

void netmgr_config_init(void) {
    // Load config from flash/nvram or use defaults based on profile
    printf("[netmgr_config] Loading base network configuration\n");

    // Mock default for edge-net-min profile
    strcpy(current_config.hostname, "edge-node-01");
    current_config.dns_server = 0x08080808; // 8.8.8.8

    strcpy(current_config.ifaces[0].name, "eth0");
    current_config.ifaces[0].use_dhcp = true; // Default to DHCP
}

netmgr_config_t* netmgr_get_config(void) {
    return &current_config;
}
