#ifndef BHARAT_VIRTIO_GPU_H
#define BHARAT_VIRTIO_GPU_H

#include <stdint.h>

/* -----------------------------------------------------------------------
 * VirtIO MMIO transport registers (offsets from base address)
 * Spec: VirtIO 1.2, Section 4.2
 * ----------------------------------------------------------------------- */
#define VIRTIO_MMIO_MAGIC_VALUE          0x000  /* 0x74726976 ("virt") */
#define VIRTIO_MMIO_VERSION              0x004  /* 2 for VirtIO 1.x    */
#define VIRTIO_MMIO_DEVICE_ID            0x008
#define VIRTIO_MMIO_VENDOR_ID            0x00C
#define VIRTIO_MMIO_DEVICE_FEATURES      0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL  0x014
#define VIRTIO_MMIO_DRIVER_FEATURES      0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL  0x024
#define VIRTIO_MMIO_QUEUE_SEL            0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX        0x034
#define VIRTIO_MMIO_QUEUE_NUM            0x038
#define VIRTIO_MMIO_QUEUE_READY          0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY         0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS     0x060
#define VIRTIO_MMIO_INTERRUPT_ACK        0x064
#define VIRTIO_MMIO_STATUS               0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW       0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH      0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW      0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH     0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW       0x0A0
#define VIRTIO_MMIO_QUEUE_USED_HIGH      0x0A4
#define VIRTIO_MMIO_CONFIG_GENERATION    0x0FC
#define VIRTIO_MMIO_CONFIG               0x100

/* VirtIO device IDs */
#define VIRTIO_ID_GPU   16

/* VirtIO status bits */
#define VIRTIO_STATUS_ACKNOWLEDGE  (1u << 0)
#define VIRTIO_STATUS_DRIVER       (1u << 1)
#define VIRTIO_STATUS_DRIVER_OK    (1u << 2)
#define VIRTIO_STATUS_FEATURES_OK  (1u << 3)
#define VIRTIO_STATUS_FAILED       (1u << 7)

/* Virtqueue descriptor flags */
#define VIRTQ_DESC_F_NEXT    1u
#define VIRTQ_DESC_F_WRITE   2u

/* virtqueue size (power of 2) */
#define VQ_SIZE  16

/* -----------------------------------------------------------------------
 * VirtIO GPU command types  (VirtIO 1.2, Section 5.7.6)
 * ----------------------------------------------------------------------- */
#define VIRTIO_GPU_CMD_GET_DISPLAY_INFO        0x0100
#define VIRTIO_GPU_CMD_RESOURCE_CREATE_2D      0x0101
#define VIRTIO_GPU_CMD_RESOURCE_UNREF          0x0102
#define VIRTIO_GPU_CMD_SET_SCANOUT             0x0103
#define VIRTIO_GPU_CMD_RESOURCE_FLUSH          0x0104
#define VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D     0x0105
#define VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING 0x0106
#define VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING 0x0107

/* VirtIO GPU response types */
#define VIRTIO_GPU_RESP_OK_NODATA    0x1100
#define VIRTIO_GPU_RESP_OK_DISPLAY_INFO 0x1101
#define VIRTIO_GPU_RESP_ERR_UNSPEC   0x1200

/* Pixel format */
#define VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM  2
#define VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM  67

/* -----------------------------------------------------------------------
 * Screen / text-console dimensions
 * ----------------------------------------------------------------------- */
#define VGPU_WIDTH   640
#define VGPU_HEIGHT  400
#define VGPU_CW      8    /* character cell width  in pixels */
#define VGPU_CH      16   /* character cell height in pixels */
#define VGPU_COLS    (VGPU_WIDTH  / VGPU_CW)   /* 80 */
#define VGPU_ROWS    (VGPU_HEIGHT / VGPU_CH)   /* 25 */

/* Resource ID (arbitrary non-zero ID for our framebuffer) */
#define VGPU_RESOURCE_ID  1

/* -----------------------------------------------------------------------
 * VirtIO GPU request/response structures  (all little-endian)
 * ----------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t flags;
    uint64_t fence_id;
    uint32_t ctx_id;
    uint32_t padding;
} virtio_gpu_ctrl_hdr_t;

typedef struct __attribute__((packed)) {
    uint32_t x, y, width, height;
} virtio_gpu_rect_t;

typedef struct __attribute__((packed)) {
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t format;
    uint32_t width;
    uint32_t height;
} virtio_gpu_resource_create_2d_t;

typedef struct __attribute__((packed)) {
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t nr_entries;
} virtio_gpu_resource_attach_backing_t;

typedef struct __attribute__((packed)) {
    uint64_t addr;
    uint32_t length;
    uint32_t padding;
} virtio_gpu_mem_entry_t;

typedef struct __attribute__((packed)) {
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_rect_t     r;
    uint32_t scanout_id;
    uint32_t resource_id;
} virtio_gpu_set_scanout_t;

typedef struct __attribute__((packed)) {
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_rect_t     r;
    uint64_t offset;
    uint32_t resource_id;
    uint32_t padding;
} virtio_gpu_transfer_to_host_2d_t;

typedef struct __attribute__((packed)) {
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_rect_t     r;
    uint32_t resource_id;
    uint32_t padding;
} virtio_gpu_resource_flush_t;

/* Generic response (used for all non-data responses) */
typedef struct __attribute__((packed)) {
    virtio_gpu_ctrl_hdr_t hdr;
} virtio_gpu_resp_nodata_t;

/* -----------------------------------------------------------------------
 * Virtqueue structures (VirtIO 1.2, Section 2.7)
 * ----------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} virtq_desc_t;

typedef struct __attribute__((packed)) {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VQ_SIZE];
} virtq_avail_t;

typedef struct __attribute__((packed)) {
    uint32_t id;
    uint32_t len;
} virtq_used_elem_t;

typedef struct __attribute__((packed)) {
    uint16_t        flags;
    uint16_t        idx;
    virtq_used_elem_t ring[VQ_SIZE];
} virtq_used_t;

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */
void virtio_gpu_init(void);
void virtio_gpu_write_char(char c);
void virtio_gpu_write_string(const char *s);

#endif /* BHARAT_VIRTIO_GPU_H */
