#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bharat/elf/elf_parser.h>

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); \
            exit(1); \
        } \
    } while(0)

static uint8_t* read_file(const char* filename, size_t* out_size) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    *out_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* data = (uint8_t*)malloc(*out_size);
    if (!data) {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(data, 1, *out_size, f);
    fclose(f);

    if (read_bytes != *out_size) {
        free(data);
        return NULL;
    }

    return data;
}

void test_golden_elf64_parsing() {
    size_t size;
    uint8_t* golden_elf = read_file("assets/elf/valid_elf64.elf", &size);
    if (!golden_elf) {
        // Fallback for CMake build dir
        golden_elf = read_file("../tests/assets/elf/valid_elf64.elf", &size);
    }
    if (!golden_elf) {
        // Further fallback for test executable execution directory
        golden_elf = read_file("../../tests/assets/elf/valid_elf64.elf", &size);
    }
    ASSERT(golden_elf != NULL, "Failed to load golden ELF test fixture");

    elf_summary_t summary;
    elf_parse_status_t status = elf_parse_image(golden_elf, size, &summary);
    ASSERT(status == ELF_PARSE_OK, "Valid ELF64 parsing failed");

    // In our generated minimal assembly, entry point is 0x401000
    ASSERT(summary.entry_point == 0x401000, "Incorrect entry point");
    ASSERT(summary.program_header_count == 2, "Incorrect program header count");

    size_t load_count;
    status = elf_get_load_segment_count(golden_elf, size, &load_count);
    ASSERT(status == ELF_PARSE_OK, "Getting load segment count failed");
    ASSERT(load_count == 2, "Incorrect load segment count");

    elf_segment_t seg[2];
    size_t written;
    status = elf_extract_load_segments(golden_elf, size, seg, 2, &written);
    ASSERT(status == ELF_PARSE_OK, "Extracting load segments failed");
    ASSERT(written == 2, "Incorrect written segment count");
    ASSERT(seg[1].virtual_address == 0x401000, "Incorrect virtual address");
    ASSERT(seg[1].file_size == 0x10, "Incorrect file size"); // The assembly instructions size padded to 16 bytes

    free(golden_elf);
}

static const uint8_t malformed_header[] = {
    0x7f, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, // e_ident
    2, 0, // e_type (ET_EXEC)
    0x3e, 0, // e_machine (AMD64)
    1, 0, 0, 0, // e_version
    0x00, 0x10, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, // e_entry
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // e_phoff
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // e_shoff
    0x00, 0x00, 0x00, 0x00, // e_flags
    0x40, 0x00, // e_ehsize
    0x38, 0x00, // e_phentsize
    1, 0, // e_phnum (1 segment)
    0x40, 0x00, // e_shentsize
    0, 0, // e_shnum
    0, 0, // e_shstrndx

    // Program Header 1 (PT_LOAD)
    1, 0, 0, 0, // p_type (PT_LOAD)
    5, 0, 0, 0, // p_flags (R E)
    0, 0, 0, 0, 0, 0, 0, 0, // p_offset
    0x00, 0x10, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, // p_vaddr
    0x00, 0x10, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, // p_paddr
    0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // p_filesz
    0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // p_memsz
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // p_align
};

void test_invalid_magic() {
    uint8_t bad_magic[sizeof(malformed_header)];
    memcpy(bad_magic, malformed_header, sizeof(malformed_header));
    bad_magic[0] = 0x00;

    elf_summary_t summary;
    elf_parse_status_t status = elf_parse_image(bad_magic, sizeof(bad_magic), &summary);
    ASSERT(status == ELF_PARSE_ERR_INVALID_MAGIC, "Expected invalid magic error");
}

void test_unsupported_class_elf32() {
    uint8_t elf32[sizeof(malformed_header)];
    memcpy(elf32, malformed_header, sizeof(malformed_header));
    elf32[4] = 1; // ELFCLASS32

    elf_summary_t summary;
    elf_parse_status_t status = elf_parse_image(elf32, sizeof(elf32), &summary);
    ASSERT(status == ELF_PARSE_ERR_UNSUPPORTED_CLASS, "Expected unsupported class error");
}

void test_unsupported_endianness() {
    uint8_t big_endian[sizeof(malformed_header)];
    memcpy(big_endian, malformed_header, sizeof(malformed_header));
    big_endian[5] = 2; // ELFDATA2MSB

    elf_summary_t summary;
    elf_parse_status_t status = elf_parse_image(big_endian, sizeof(big_endian), &summary);
    ASSERT(status == ELF_PARSE_ERR_UNSUPPORTED_ENDIANNESS, "Expected unsupported endianness error");
}

void test_buffer_too_small() {
    elf_summary_t summary;
    elf_parse_status_t status = elf_parse_image(malformed_header, 10, &summary);
    ASSERT(status == ELF_PARSE_ERR_BUFFER_TOO_SMALL, "Expected buffer too small error");
}

int main() {
    printf("Testing ELF parser...\n");
    test_golden_elf64_parsing();
    test_invalid_magic();
    test_unsupported_class_elf32();
    test_unsupported_endianness();
    test_buffer_too_small();
    printf("ELF parser tests passed.\n");
    return 0;
}
