#ifndef BHARAT_DRIVERS_BUS_PCIE_HOST_H
#define BHARAT_DRIVERS_BUS_PCIE_HOST_H

#include <stdint.h>

/*
 * Minimal PCIe host bridge contract.
 *
 * This is intentionally small and self-contained so it can be enabled
 * without coupling to existing in-flight driver work.
 */

typedef struct {
    uint8_t bus_start;
    uint8_t bus_end;
    uintptr_t ecam_base;
} pcie_host_config_t;

int pcie_host_init(const pcie_host_config_t* cfg);
int pcie_host_enumerate(uint8_t* discovered_devices);

#endif
