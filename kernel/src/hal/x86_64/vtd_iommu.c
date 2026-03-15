#include "hal/iommu.h"
#include <stddef.h>

#include "hal/hal.h"
#include "hal/hal_discovery.h"
#include "device/irq_domain.h"
#include "profile.h"

// Basic VT-d discovery via hal_discovery
static bool dmar_found = false;
static bool irq_remap_supported = false;
static iommu_desc_t vtd_desc;

// Forward declaration of MSI domain operations for VT-d Interrupt Remapping
static int vtd_alloc_msi(msi_domain_t* domain, void* device, int count, msi_desc_t* desc_array);
static void vtd_free_msi(msi_domain_t* domain, msi_desc_t* desc_array, int count);
static int vtd_write_msg(msi_domain_t* domain, msi_desc_t* desc);

static msi_domain_t vtd_msi_domain = {
    .name = "vtd-irq-remap",
    .base_domain = NULL,
    .alloc_msi = vtd_alloc_msi,
    .free_msi = vtd_free_msi,
    .write_msg = vtd_write_msg,
    .host_data = NULL
};

static int vtd_init(void) {
    system_discovery_t* disc = hal_get_system_discovery();
    if (!disc) return -1;

    for (uint32_t i = 0; i < disc->iommu_count; i++) {
        if (disc->iommus[i].type == IOMMU_VTD) {
            dmar_found = true;
            vtd_desc = disc->iommus[i];

            // Bit 0 of DMAR flags is INTR_REMAP
            if (vtd_desc.flags & 0x01) {
                irq_remap_supported = true;
                msi_domain_register(&vtd_msi_domain);
            }
            break;
        }
    }

    if (!dmar_found) {
        return -1;
    }

    // Initialize DRHD registers, establish root entries and context tables
    // (Hardware programming omitted for stub)

    return 0;
}

static int vtd_alloc_msi(msi_domain_t* domain, void* device, int count, msi_desc_t* desc_array) {
    (void)domain;
    (void)device;
    if (!irq_remap_supported) return -1;

    // Allocate Interrupt Remapping Table Entries (IRTE)
    // Populate desc_array with virtual IRQs
    for (int i = 0; i < count; i++) {
        desc_array[i].irq = 0x100 + i; // Stub VIRQ
        // MSI message for VT-d Interrupt Remapping format
        // Address points to VT-d remap window with Handle (IRTE index)
        desc_array[i].msg.address = 0xFEE00000 | (1 << 4) /* SHV */ | (1 << 3) /* Remapped */;
        desc_array[i].msg.data = i; // The IRTE index
    }
    return 0;
}

static void vtd_free_msi(msi_domain_t* domain, msi_desc_t* desc_array, int count) {
    (void)domain;
    (void)desc_array;
    (void)count;
    // Clear IRTEs
}

static int vtd_write_msg(msi_domain_t* domain, msi_desc_t* desc) {
    (void)domain;
    (void)desc;
    // Modify IRTE if affinity changes
    return 0;
}

static int vtd_domain_create(const bharat_iommu_domain_config_t *cfg, hal_iommu_domain_t **out) {
    // Set up domain structures
    (void)cfg;
    if (out) *out = (hal_iommu_domain_t*)0x1234;
    return 0;
}

static int vtd_domain_destroy(hal_iommu_domain_t *domain) {
    (void)domain;
    return 0;
}

static int vtd_group_attach(hal_iommu_group_t *group, hal_iommu_domain_t *domain) {
    (void)group;
    (void)domain;
    return 0;
}

static int vtd_group_detach(hal_iommu_group_t *group) {
    (void)group;
    return 0;
}

static int vtd_map(hal_iommu_domain_t *domain, uint64_t iova, uint64_t phys, size_t size, uint64_t prot) {
    (void)domain;
    (void)iova;
    (void)phys;
    (void)size;
    (void)prot;
    return 0;
}

static int vtd_unmap(hal_iommu_domain_t *domain, uint64_t iova, size_t size) {
    (void)domain;
    (void)iova;
    (void)size;
    return 0;
}

static int vtd_block_device(bharat_device_t *dev) {
    (void)dev;
    return 0;
}

static int vtd_get_group(bharat_device_t *dev, hal_iommu_group_t **out_group) {
    (void)dev;
    if (out_group) *out_group = (hal_iommu_group_t*)0x5678;
    return 0;
}

static hal_iommu_ops_t vtd_ops = {
    .init = vtd_init,
    .domain_create = vtd_domain_create,
    .domain_destroy = vtd_domain_destroy,
    .group_attach = vtd_group_attach,
    .group_detach = vtd_group_detach,
    .map = vtd_map,
    .unmap = vtd_unmap,
    .block_device = vtd_block_device,
    .get_group = vtd_get_group
};

void x86_iommu_detect(void) {
    // IOMMU backends must be discovered via firmware/board descriptors (ACPI)
    // rather than ad-hoc probing.
    // Simulation: check if ACPI has DMAR
    if (vtd_init() == 0) {
        hal_iommu_register_ops(&vtd_ops);
    } else {
        // Policy-driven degradation
        // e.g. blocking untrusted DMA devices in high isolation profiles
        // allowing trusted-only operation in edge profiles.
        // Isolation layer handles policy, we just don't register ops.
    }
}
