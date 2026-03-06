#ifndef BHARAT_HAL_GPU_H
#define BHARAT_HAL_GPU_H

#include "hal/hal.h"
#include "../advanced/formal_verif.h"

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

typedef enum {
    GPU_API_NONE = 0,
    GPU_API_VULKAN = 1,
    GPU_API_MODERN_EXPLICIT = 2
} gpu_api_t;

typedef enum {
    GPU_PLANE_PRIMARY = 0,
    GPU_PLANE_OVERLAY = 1,
    GPU_PLANE_CURSOR = 2
} gpu_plane_t;

typedef struct {
    uint64_t phys_addr;
    uint64_t size_bytes;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;
} gpu_buffer_t;

// Discover available GPUs on the system bus (e.g., PCIe)
int hal_gpu_enumerate(gpu_device_t* devices, int max_devices);

// Initialize a specific GPU context
int hal_gpu_init(gpu_device_t* gpu, capability_t* cap);

// Query preferred user-space render API for this device (Vulkan-class expected)
int hal_gpu_get_supported_api(gpu_device_t* gpu, gpu_api_t* out_api, capability_t* cap);

// Basic graphics output (Framebuffer) if supported
int hal_gpu_set_mode(gpu_device_t* gpu, uint32_t width, uint32_t height, uint32_t bpp);
void* hal_gpu_get_framebuffer(gpu_device_t* gpu);

// Allocate/share scanout-capable buffers with capability-controlled zero-copy mapping
int hal_gpu_alloc_shared_buffer(gpu_device_t* gpu, gpu_buffer_t* out_buffer, uint32_t width, uint32_t height, uint32_t format, capability_t* cap);
int hal_gpu_map_shared_buffer(gpu_device_t* gpu, gpu_buffer_t* buffer, void** out_user_ptr, capability_t* cap);
int hal_gpu_present_buffer(gpu_device_t* gpu, gpu_buffer_t* buffer, capability_t* cap);

// Optional compositing fast-path for simple overlay/cursor planes
int hal_gpu_bind_plane(gpu_device_t* gpu, gpu_plane_t plane, gpu_buffer_t* buffer, capability_t* cap);

// Submit raw compute queue standard instruction buffer (abstracted OpenCL/Vulkan backend queue)
int hal_gpu_submit_compute(gpu_device_t* gpu, void* cmd_buffer, uint32_t size, capability_t* cap);

#endif // BHARAT_HAL_GPU_H
