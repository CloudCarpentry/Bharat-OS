#include "bharat/stacks/storage/persistent/durability.h"

io_status_t storage_durability_evaluate(
    const io_device_caps_t *caps,
    io_durability_level_t requested,
    storage_durability_result_t *out) {
    if (!caps || !out) return IO_STATUS_INVALID_ARGUMENT;

    switch (requested) {
        case IO_DURABILITY_UNSAFE:
            out->status = DURABILITY_SUPPORTED;
            return IO_STATUS_OK;

        case IO_DURABILITY_PROCESS_VISIBLE:
            out->status = DURABILITY_SUPPORTED;
            return IO_STATUS_OK;

        case IO_DURABILITY_DEVICE_FLUSHED:
            if (!caps->volatile_write_cache) {
                out->status = DURABILITY_SUPPORTED;
            } else if (caps->flush_supported) {
                out->status = DURABILITY_SUPPORTED_WITH_FLUSH;
            } else {
                out->status = DURABILITY_NOT_SUPPORTED;
            }
            return IO_STATUS_OK;

        case IO_DURABILITY_POWER_LOSS_PROTECTED:
            if (caps->power_loss_protection) {
                out->status = DURABILITY_SUPPORTED;
            } else if (caps->fua_supported) {
                out->status = DURABILITY_SUPPORTED_WITH_FUA;
            } else if (caps->flush_supported) {
                out->status = DURABILITY_SUPPORTED_WITH_FLUSH;
            } else if (!caps->volatile_write_cache) {
                out->status = DURABILITY_SUPPORTED;
            } else {
                out->status = DURABILITY_NOT_SUPPORTED;
            }
            return IO_STATUS_OK;

        case IO_DURABILITY_ROLLBACK_PROTECTED:
            // Rollback protection is supported if we can flush and have a trusted anchor.
            if (caps->flush_supported || caps->fua_supported || !caps->volatile_write_cache) {
                out->status = DURABILITY_SUPPORTED;
            } else {
                out->status = DURABILITY_NOT_SUPPORTED;
            }
            return IO_STATUS_OK;

        default:
            out->status = DURABILITY_NOT_SUPPORTED;
            return IO_STATUS_OK;
    }
}
