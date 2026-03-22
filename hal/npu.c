#include "../../include/hal/npu.h"
#include "../../include/mm.h"
#include "../../include/advanced/formal_verif.h"
#include <stddef.h>

/*
 * Bharat-OS Generic NPU HAL Implementation
 *
 * This file now acts as a thin capability gate + platform hook dispatcher.
 * Platform ports should override the weak symbols below with real discovery,
 * MMIO mapping and queue submission logic.
 */

int __attribute__((weak)) hal_platform_npu_enumerate(npu_device_t* devices, int max_devices) {
    (void)devices;
    (void)max_devices;
    return 0;
}

int __attribute__((weak)) hal_platform_npu_init(npu_device_t* npu, capability_t* cap) {
    (void)npu;
    (void)cap;
    return -3; // Not implemented by platform
}

int __attribute__((weak)) hal_platform_npu_load_model(npu_device_t* npu,
                                                       void* model_data,
                                                       uint64_t size,
                                                       uint32_t* model_handle_out,
                                                       capability_t* cap) {
    (void)npu;
    (void)model_data;
    (void)size;
    (void)model_handle_out;
    (void)cap;
    return -3; // Not implemented by platform
}

int __attribute__((weak)) hal_platform_npu_queue_inference(npu_device_t* npu,
                                                            uint32_t model_handle,
                                                            void* input_tensors,
                                                            void* output_tensors,
                                                            capability_t* cap) {
    (void)npu;
    (void)model_handle;
    (void)input_tensors;
    (void)output_tensors;
    (void)cap;
    return -3; // Not implemented by platform
}

int hal_npu_enumerate(npu_device_t* devices, int max_devices) {
    if (!devices || max_devices <= 0) return -1;

    return hal_platform_npu_enumerate(devices, max_devices);
}

int hal_npu_init(npu_device_t* npu, capability_t* cap) {
    if (!npu || !cap) return -1;

    // Capability-Gated Access Check
    if ((cap->rights_mask & CAP_RIGHT_DEVICE_NPU) == 0) {
        return -2; // Unauthorized access
    }

    return hal_platform_npu_init(npu, cap);
}

int hal_npu_load_model(npu_device_t* npu,
                       void* model_data,
                       uint64_t size,
                       uint32_t* model_handle_out,
                       capability_t* cap) {
    if (!npu || !cap || !model_data || size == 0u) return -1;

    // Capability-Gated Access Check
    if ((cap->rights_mask & CAP_RIGHT_DEVICE_NPU) == 0) {
        return -2; // Unauthorized access
    }

    return hal_platform_npu_load_model(npu, model_data, size, model_handle_out, cap);
}

int hal_npu_queue_inference(npu_device_t* npu,
                            uint32_t model_handle,
                            void* input_tensors,
                            void* output_tensors,
                            capability_t* cap) {
    if (!npu || !cap || !input_tensors || !output_tensors) return -1;

    // Capability-Gated Access Check
    if ((cap->rights_mask & CAP_RIGHT_DEVICE_NPU) == 0) {
        return -2; // Unauthorized access
    }

    return hal_platform_npu_queue_inference(npu, model_handle, input_tensors, output_tensors, cap);
}
