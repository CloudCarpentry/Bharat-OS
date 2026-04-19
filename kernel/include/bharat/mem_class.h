#ifndef BHARAT_MEM_CLASS_H
#define BHARAT_MEM_CLASS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Memory Class Tags.
 * Broad classification of memory requests allowing advanced hardware and policy
 * engines to route buffers to the correct allocator and backend pool.
 * This lives above specific allocator implementations like PMM/VMM.
 */
typedef enum alloc_class {
    MEM_NORMAL = 0,
    MEM_DMA,
    MEM_RT,
    MEM_SECURE,
    MEM_PACKET,
    MEM_LOWPOWER,
    MEM_PERSISTENT,
    /* Ensure max classes is tracked in statistics limits */
    MEM_CLASS_MAX = 8
} alloc_class_t;

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_MEM_CLASS_H */
