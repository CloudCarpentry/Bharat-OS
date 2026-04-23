#include "mm/iommu.h"
#include "hal/hal_iommu.h"
#include "hal/hal_irq.h"
#include "console/console_core.h"
#include "panic.h"

void iommu_report_fault(iommu_fault_info_t *info);

static irq_return_t iommu_fault_handler(void* ctx) {
    (void)ctx;
    // In a real implementation, we would read the fault info from hardware registers.
    // For now, we simulate a fault info object.
    iommu_fault_info_t info = {
        .dev = NULL,
        .iova = 0xDEADBEEF,
        .access = 0,
        .reason = 0,
        .recoverable = false
    };
    iommu_report_fault(&info);
    return IRQ_HANDLED;
}

int iommu_fault_init(uint32_t irq_vector) {
    return hal_interrupt_register(irq_vector, iommu_fault_handler, NULL, 0, "iommu_fault", NULL);
}

void iommu_report_fault(iommu_fault_info_t *info) {
    if (!info) return;

    // Log the fault explicitly
    console_write_raw("IOMMU: Fault detected - device bypass/reject violation on IOVA: ", 64);
    // We don't have a reliable snprintf exported cleanly across all profiles
    // without including `lib/string.h` (if it implements it) or specific helpers.
    // Basic hex output to console using HAL or standard console.
    extern void hal_serial_write_hex(uint64_t val);
    hal_serial_write_hex((uint64_t)info->iova);
    console_write_raw("\n", 1);

    if (!info->recoverable) {
        kernel_panic("IOMMU: Fatal DMA isolation violation.");
    }
}
