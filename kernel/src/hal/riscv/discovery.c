#include "hal/hal_discovery.h"
#include "hal/fdt_parser.h"
#include "bharat/boot_info.h"

void hal_arch_discovery_init(const boot_info_t *boot) {
    if (!boot || !boot->arch_ptr) return;

    system_discovery_t *discovery = hal_get_system_discovery();
    fdt_parse_discovery(boot->arch_ptr, discovery);
}
