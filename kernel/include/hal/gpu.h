#ifndef BHARAT_HAL_GPU_H
#define BHARAT_HAL_GPU_H

#include "hal/hal.h"

/* 
 * GPU Hardware Abstraction Layer Base 
 * GPUs are treated as peripheral compute nodes in Bharat-OS.
 */

typedef struct {
    uint32_t vendor_id;
    uint32_t device_id;
    uint64_t vram_size_bytes;
    void* mmio_base;
} gpu_device_t;

// Discover available GPUs on the system bus (e.g., PCIe)
int hal_gpu_enumerate(gpu_device_t* devices, int max_devices);

// Initialize a specific GPU context
int hal_gpu_init(gpu_device_t* gpu);

// Basic graphics output (Framebuffer) if supported
int hal_gpu_set_mode(gpu_device_t* gpu, uint32_t width, uint32_t height, uint32_t bpp);
void* hal_gpu_get_framebuffer(gpu_device_t* gpu);

// Submit raw compute queue standard instruction buffer (abstracted OpenCL/Vulkan backend queue)
int hal_gpu_submit_compute(gpu_device_t* gpu, void* cmd_buffer, uint32_t size);

#endif // BHARAT_HAL_GPU_H
