#ifndef BHARAT_ELF_PARSER_H
#define BHARAT_ELF_PARSER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Note: Only ELF64 and Little-Endian are supported in this version.
 */

typedef enum {
    ELF_PARSE_OK = 0,
    ELF_PARSE_ERR_INVALID_MAGIC,
    ELF_PARSE_ERR_UNSUPPORTED_CLASS,
    ELF_PARSE_ERR_UNSUPPORTED_ENDIANNESS,
    ELF_PARSE_ERR_INVALID_VERSION,
    ELF_PARSE_ERR_BUFFER_TOO_SMALL,
    ELF_PARSE_ERR_MALFORMED,
} elf_parse_status_t;

typedef struct {
    uint64_t entry_point;
    uint16_t program_header_count;
    uint64_t program_header_offset;
    uint16_t program_header_entry_size;
} elf_summary_t;

typedef struct {
    uint64_t virtual_address;
    uint64_t physical_address;
    uint64_t file_offset;
    uint64_t file_size;
    uint64_t memory_size;
    uint32_t flags;
    uint64_t alignment;
} elf_segment_t;

/**
 * Validates the ELF image and extracts a high-level summary.
 */
elf_parse_status_t elf_parse_image(const uint8_t* bytes, size_t size, elf_summary_t* summary);

/**
 * Gets the number of PT_LOAD segments in the ELF image.
 */
elf_parse_status_t elf_get_load_segment_count(const uint8_t* bytes, size_t size, size_t* count);

/**
 * Extracts all PT_LOAD segments from the ELF image into the provided array.
 */
elf_parse_status_t elf_extract_load_segments(const uint8_t* bytes, size_t size, elf_segment_t* segments, size_t capacity, size_t* written);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_ELF_PARSER_H */
