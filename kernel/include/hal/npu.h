#ifndef BHARAT_HAL_NPU_H
#define BHARAT_HAL_NPU_H

#include "hal/hal.h"
#include "../advanced/formal_verif.h"

/* 
 * NPU (Neural Processing Unit) Hardware Abstraction Layer
 * NPUs are highly specialized asynchronous compute units designed for tensor operations.
 */

typedef struct {
    uint32_t vendor_id;
    uint32_t device_id;
    uint64_t memory_size_bytes;
    uint32_t ops_per_sec_estimate;
} npu_device_t;

// Discover NPUs on the system
int hal_npu_enumerate(npu_device_t* devices, int max_devices);

// Initialize power/clocks/context for NPU
int hal_npu_init(npu_device_t* npu, capability_t* cap);

// Load a serialized model/graph payload to the NPU
int hal_npu_load_model(npu_device_t* npu, void* model_data, uint64_t size, uint32_t* model_handle_out, capability_t* cap);

// Queue inference instruction
int hal_npu_queue_inference(npu_device_t* npu, uint32_t model_handle, void* input_tensors, void* output_tensors, capability_t* cap);

#endif // BHARAT_HAL_NPU_H
