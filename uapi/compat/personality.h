#ifndef BHARAT_UAPI_COMPAT_PERSONALITY_H
#define BHARAT_UAPI_COMPAT_PERSONALITY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Core personality identification.
 * Defines the OS personality bound to a process or thread.
 */
typedef enum {
    BHARAT_PERSONALITY_NATIVE = 0,
    BHARAT_PERSONALITY_LINUX,
    BHARAT_PERSONALITY_ANDROID,
} bharat_personality_id_t;

/**
 * @brief Classification of system calls for performance routing.
 */
typedef enum {
    SYSCALL_CLASS_FAST,
    SYSCALL_CLASS_VM,
    SYSCALL_CLASS_SYNC,
    SYSCALL_CLASS_IO,
    SYSCALL_CLASS_SIGNAL,
    SYSCALL_CLASS_SLOW,
} syscall_class_t;

/**
 * @brief Fast-path eligibility rules for a syscall.
 */
typedef enum {
    SYSCALL_MAP_DIRECT,
    SYSCALL_MAP_TRANSLATED,
    SYSCALL_MAP_EMULATED,
} syscall_map_type_t;

/**
 * @brief Process-level personality descriptor.
 * Attached to the process/task creation path to avoid scattering ABI assumptions.
 */
typedef struct {
    bharat_personality_id_t personality;
    uint8_t abi_bits;       /* e.g., 32 or 64 */
    uint8_t signal_model;   /* Specifies how signals are routed/delivered */
    uint8_t tls_model;      /* TLS initialization and layout semantics */
} personality_process_desc_t;

/**
 * @brief Thread-level personality descriptor.
 * Carries thread-specific ABI and compat logic (e.g., futex semantics).
 */
typedef struct {
    uint64_t tls_base;
    uint32_t futex_compat;  /* Futex semantics version or mapping flag */
} personality_thread_desc_t;

/**
 * @brief Memory Management (VM) personality descriptor.
 * Maps Linux-style mmap semantics and copy-on-write models securely.
 */
typedef struct {
    uint8_t mmap_semantics; /* Handles private/shared/anon behaviors */
    uint8_t cow_model;      /* Copy-on-write mapping logic */
} personality_mm_desc_t;

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_COMPAT_PERSONALITY_H */
