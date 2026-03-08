#ifndef BHARAT_ACCEL_H
#define BHARAT_ACCEL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Accelerator Device Capabilities Bitmasks
 * Negotiate features and ISA extensions (e.g., SIMD, DMA, NPU)
 */
typedef enum {
    BHARAT_ACCEL_CAP_NONE      = 0,
    BHARAT_ACCEL_CAP_DMA       = 1 << 0, // General Purpose DMA
    BHARAT_ACCEL_CAP_BLIT      = 1 << 1, // 2D Display Blitter (Fill, Copy)
    BHARAT_ACCEL_CAP_SCALE     = 1 << 2, // Image scaler
    BHARAT_ACCEL_CAP_ROT       = 1 << 3, // Image rotation engine
    BHARAT_ACCEL_CAP_DSP       = 1 << 4, // Digital Signal Processor
    BHARAT_ACCEL_CAP_NPU       = 1 << 5, // Neural Processing Unit
    BHARAT_ACCEL_CAP_CRYPTO    = 1 << 6, // Hardware Crypto offload
    BHARAT_ACCEL_CAP_SIMD_NEON = 1 << 7, // ARM NEON SIMD optimizations
    BHARAT_ACCEL_CAP_SIMD_AVX  = 1 << 8, // x86_64 AVX optimizations
    BHARAT_ACCEL_CAP_SIMD_RVV  = 1 << 9, // RISC-V Vector Extensions (RVV)
} bharat_accel_capability_t;

/**
 * Blit operation payload
 */
typedef struct {
    void *src_addr;
    void *dst_addr;
    uint32_t width;
    uint32_t height;
    uint32_t src_stride;
    uint32_t dst_stride;
    uint32_t bpp;
    bool enable_alpha;
    uint32_t fill_color;
} bharat_accel_blit_req_t;

struct bharat_accel_device;

/**
 * Accelerator Operations
 */
typedef struct {
    int (*init)(struct bharat_accel_device *dev);
    void (*deinit)(struct bharat_accel_device *dev);

    // Core DMA
    int (*dma_memcpy)(struct bharat_accel_device *dev, void *dst, const void *src, size_t n);

    // Display Blitter
    int (*blit)(struct bharat_accel_device *dev, const bharat_accel_blit_req_t *req);
    int (*fill)(struct bharat_accel_device *dev, void *dst, uint32_t color, uint32_t w, uint32_t h, uint32_t stride);

    // Advanced compute
    int (*submit_npu_job)(struct bharat_accel_device *dev, void *job_descriptor);
} bharat_accel_device_ops_t;

/**
 * Accelerator Device Registration
 */
typedef struct bharat_accel_device {
    const char *name;
    uint32_t id;
    uint32_t capabilities; // Bitmask of BHARAT_ACCEL_CAP_*
    const bharat_accel_device_ops_t *ops;
    void *priv_data;
} bharat_accel_device_t;

// API
int bharat_accel_register(bharat_accel_device_t *dev);
uint32_t bharat_accel_get_global_caps(void);
bharat_accel_device_t* bharat_accel_find_by_cap(bharat_accel_capability_t cap);

#endif // BHARAT_ACCEL_H
