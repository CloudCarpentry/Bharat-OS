#include "pcie_host.h"

#include <stddef.h>

static pcie_host_config_t g_pcie_host_cfg;
static int g_pcie_host_initialized;

int pcie_host_init(const pcie_host_config_t* cfg)
{
    if (cfg == NULL) {
        return -1;
    }

    if ((cfg->bus_end < cfg->bus_start) || (cfg->ecam_base == 0u)) {
        return -2;
    }

    g_pcie_host_cfg = *cfg;
    g_pcie_host_initialized = 1;
    return 0;
}

int pcie_host_enumerate(uint8_t* discovered_devices)
{
    if (!g_pcie_host_initialized || discovered_devices == NULL) {
        return -1;
    }

    /*
     * Non-invasive placeholder:
     * keep in-flight code unaffected by returning a deterministic value
     * until full ECAM scan and BAR sizing support are added.
     */
    *discovered_devices = 0u;
    (void)g_pcie_host_cfg;
    return 0;
}
