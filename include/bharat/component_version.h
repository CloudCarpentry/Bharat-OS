#ifndef BHARAT_COMPONENT_VERSION_H
#define BHARAT_COMPONENT_VERSION_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BHARAT_NAME_MAX 32
#define BHARAT_VERSTR_MAX 32
#define BHARAT_HASH_MAX 48
#define BHARAT_TIME_MAX 32
#define BHARAT_KEYID_MAX 32

/* Standardized metadata structure for Bharat-OS components */
/* Using a flat POD record to eliminate pointer relocations in PIE/freestanding builds */
struct bharat_component_version {
    uint32_t magic;           /* 0x42565253 = 'BVRS' */
    uint16_t struct_version;
    uint16_t size;

    uint32_t iface_version;
    uint32_t abi_version;

    uint32_t is_dirty;
    uint64_t build_epoch;

    char name[BHARAT_NAME_MAX];
    char kind[16];
    char version[BHARAT_VERSTR_MAX];
    char channel[16];
    char git_sha[BHARAT_HASH_MAX];
    char build_time_utc[BHARAT_TIME_MAX];
};

/* Macro to embed the version block into the .bharat.version ELF section */
#define BHARAT_REGISTER_COMPONENT(name_, kind_, version_, iface_version_, abi_version_, channel_, git_sha_, is_dirty_, build_epoch_, build_time_utc_) \
    __attribute__((used, section(".bharat.version"), aligned(16))) \
    const struct bharat_component_version _bharat_component_info = { \
        .magic = 0x42565253, \
        .struct_version = 1, \
        .size = sizeof(struct bharat_component_version), \
        .iface_version = iface_version_, \
        .abi_version = abi_version_, \
        .is_dirty = is_dirty_, \
        .build_epoch = build_epoch_, \
        .name = name_, \
        .kind = kind_, \
        .version = version_, \
        .channel = channel_, \
        .git_sha = git_sha_, \
        .build_time_utc = build_time_utc_ \
    }

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_COMPONENT_VERSION_H */
