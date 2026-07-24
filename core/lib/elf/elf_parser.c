#include <bharat/elf/elf_parser.h>

/* ELF Header */
#define EI_NIDENT       16
#define EI_MAG0         0
#define EI_MAG1         1
#define EI_MAG2         2
#define EI_MAG3         3
#define EI_CLASS        4
#define EI_DATA         5
#define EI_VERSION      6

#define ELFMAG0         0x7f
#define ELFMAG1         'E'
#define ELFMAG2         'L'
#define ELFMAG3         'F'

#define ELFCLASSNONE    0
#define ELFCLASS32      1
#define ELFCLASS64      2

#define ELFDATANONE     0
#define ELFDATA2LSB     1
#define ELFDATA2MSB     2

#define EV_CURRENT      1

#define ET_EXEC         2
#define ET_DYN          3

#define PT_LOAD         1

typedef struct {
    uint8_t   e_ident[EI_NIDENT];
    uint16_t  e_type;
    uint16_t  e_machine;
    uint32_t  e_version;
    uint64_t  e_entry;
    uint64_t  e_phoff;
    uint64_t  e_shoff;
    uint32_t  e_flags;
    uint16_t  e_ehsize;
    uint16_t  e_phentsize;
    uint16_t  e_phnum;
    uint16_t  e_shentsize;
    uint16_t  e_shnum;
    uint16_t  e_shstrndx;
} elf64_ehdr_t;

typedef struct {
    uint32_t  p_type;
    uint32_t  p_flags;
    uint64_t  p_offset;
    uint64_t  p_vaddr;
    uint64_t  p_paddr;
    uint64_t  p_filesz;
    uint64_t  p_memsz;
    uint64_t  p_align;
} elf64_phdr_t;

elf_parse_status_t elf_parse_image(const uint8_t* bytes, size_t size, elf_summary_t* summary) {
    if (!bytes || !summary) return ELF_PARSE_ERR_MALFORMED;

    if (size < sizeof(elf64_ehdr_t)) {
        return ELF_PARSE_ERR_BUFFER_TOO_SMALL;
    }

    const elf64_ehdr_t* ehdr = (const elf64_ehdr_t*)bytes;

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        return ELF_PARSE_ERR_INVALID_MAGIC;
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        return ELF_PARSE_ERR_UNSUPPORTED_CLASS;
    }

    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        return ELF_PARSE_ERR_UNSUPPORTED_ENDIANNESS;
    }

    if (ehdr->e_ident[EI_VERSION] != EV_CURRENT || ehdr->e_version != EV_CURRENT) {
        return ELF_PARSE_ERR_INVALID_VERSION;
    }

    if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
        /* Rejecting object files or cores. */
        return ELF_PARSE_ERR_MALFORMED;
    }

    if (ehdr->e_phentsize < sizeof(elf64_phdr_t)) {
        return ELF_PARSE_ERR_MALFORMED;
    }

    /* Check for overflow in program header parsing */
    uint64_t ph_table_size = (uint64_t)ehdr->e_phentsize * ehdr->e_phnum;
    if (ehdr->e_phoff > size || ph_table_size > size - ehdr->e_phoff) {
        return ELF_PARSE_ERR_MALFORMED;
    }

    summary->entry_point = ehdr->e_entry;
    summary->program_header_count = ehdr->e_phnum;
    summary->program_header_offset = ehdr->e_phoff;
    summary->program_header_entry_size = ehdr->e_phentsize;

    return ELF_PARSE_OK;
}

elf_parse_status_t elf_get_load_segment_count(const uint8_t* bytes, size_t size, size_t* count) {
    if (!count) return ELF_PARSE_ERR_MALFORMED;

    elf_summary_t summary;
    elf_parse_status_t status = elf_parse_image(bytes, size, &summary);
    if (status != ELF_PARSE_OK) {
        return status;
    }

    size_t load_count = 0;
    for (uint16_t i = 0; i < summary.program_header_count; ++i) {
        uint64_t offset = summary.program_header_offset + (i * summary.program_header_entry_size);
        const elf64_phdr_t* phdr = (const elf64_phdr_t*)(bytes + offset);

        if (phdr->p_type == PT_LOAD) {
            load_count++;
        }
    }

    *count = load_count;
    return ELF_PARSE_OK;
}

elf_parse_status_t elf_extract_load_segments(const uint8_t* bytes, size_t size, elf_segment_t* segments, size_t capacity, size_t* written) {
    if (!segments || !written) return ELF_PARSE_ERR_MALFORMED;
    *written = 0;

    elf_summary_t summary;
    elf_parse_status_t status = elf_parse_image(bytes, size, &summary);
    if (status != ELF_PARSE_OK) {
        return status;
    }

    size_t load_count = 0;
    for (uint16_t i = 0; i < summary.program_header_count; ++i) {
        if (load_count >= capacity) {
            break;
        }

        uint64_t offset = summary.program_header_offset + (i * summary.program_header_entry_size);
        const elf64_phdr_t* phdr = (const elf64_phdr_t*)(bytes + offset);

        if (phdr->p_type == PT_LOAD) {
            // Validate segment bounds against file
            if (phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset) {
                return ELF_PARSE_ERR_MALFORMED;
            }

            // A valid PT_LOAD segment must have memory size >= file size
            if (phdr->p_memsz < phdr->p_filesz) {
                return ELF_PARSE_ERR_MALFORMED;
            }

            segments[load_count].virtual_address = phdr->p_vaddr;
            segments[load_count].physical_address = phdr->p_paddr;
            segments[load_count].file_offset = phdr->p_offset;
            segments[load_count].file_size = phdr->p_filesz;
            segments[load_count].memory_size = phdr->p_memsz;
            segments[load_count].flags = phdr->p_flags;
            segments[load_count].alignment = phdr->p_align;

            // basic check alignment
            if (phdr->p_align != 0 && phdr->p_align != 1) {
                // Must be a power of two
                if ((phdr->p_align & (phdr->p_align - 1)) != 0) {
                    return ELF_PARSE_ERR_MALFORMED;
                }
                if ((phdr->p_vaddr % phdr->p_align) != (phdr->p_offset % phdr->p_align)) {
                    return ELF_PARSE_ERR_MALFORMED;
                }
            }

            load_count++;
        }
    }

    *written = load_count;
    return ELF_PARSE_OK;
}
