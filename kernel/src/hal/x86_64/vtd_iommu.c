#include "hal/iommu.h"
#include <stddef.h>

#include "hal/hal.h"
#include "profile.h"

// Basic VT-d discovery via ACPI DMAR parsing stub
static bool dmar_found = false;

static int vtd_init(void) {
    // Locate DMAR via ACPI
    // Stub: Check if DMAR table exists in ACPI
    // In actual implementation, parse the ACPI DMAR table.
    // If not found, use policy-driven degradation.

    // Simulating ACPI probe
    dmar_found = true; // Assume found for testing purposes, but check profile

    if (!dmar_found) {
        // Degrade to no-IOMMU based on profile policy
        return -1;
    }

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
