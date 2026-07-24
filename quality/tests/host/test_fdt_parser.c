#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "hal/fdt_parser.h"

// FDT specific constants
#define FDT_MAGIC 0xd00dfeed
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

static inline uint32_t cpu_to_fdt32(uint32_t val) {
  return ((val >> 24) & 0xff) | ((val >> 8) & 0xff00) | ((val & 0xff00) << 8) |
         ((val & 0xff) << 24);
}

void hal_serial_write(const char *s) {
}

void hal_serial_write_hex(uintptr_t x) {
}

int main() {
    printf("Starting FDT Parser advanced bounds tests\n");

    uint8_t buffer[512];
    system_discovery_t disco;
    struct fdt_header *fdt;
    uint32_t *p;
    char *str_table;

    // Helper macro to init fdt block
    #define INIT_FDT() \
        memset(buffer, 0, sizeof(buffer)); \
        memset(&disco, 0, sizeof(disco)); \
        fdt = (struct fdt_header *)buffer; \
        fdt->magic = cpu_to_fdt32(FDT_MAGIC); \
        fdt->totalsize = cpu_to_fdt32(sizeof(buffer)); \
        fdt->off_dt_struct = cpu_to_fdt32(sizeof(struct fdt_header)); \
        fdt->off_dt_strings = cpu_to_fdt32(256); \
        p = (uint32_t *)(buffer + sizeof(struct fdt_header)); \
        str_table = (char *)(buffer + 256);

    // Case 1: Valid structure
    INIT_FDT();
    *p++ = cpu_to_fdt32(FDT_BEGIN_NODE);
    strcpy((char *)p, "node1"); // 6 bytes -> 8 aligned
    p += 2;
    *p++ = cpu_to_fdt32(FDT_END_NODE);
    *p++ = cpu_to_fdt32(FDT_END);
    fdt->size_dt_struct = cpu_to_fdt32((uint8_t*)p - (buffer + sizeof(struct fdt_header)));

    int res = fdt_parse_discovery(buffer, &disco);
    if (res != 0) {
        printf("Test 1 Failed: Valid FDT should parse correctly.\n");
        return 1;
    }

    // Case 2: Zero-length node name
    INIT_FDT();
    *p++ = cpu_to_fdt32(FDT_BEGIN_NODE);
    strcpy((char *)p, ""); // 1 byte -> 4 aligned
    p += 1;
    *p++ = cpu_to_fdt32(FDT_END_NODE);
    *p++ = cpu_to_fdt32(FDT_END);
    fdt->size_dt_struct = cpu_to_fdt32((uint8_t*)p - (buffer + sizeof(struct fdt_header)));

    res = fdt_parse_discovery(buffer, &disco);
    if (res != 0) {
        printf("Test 2 Failed: Zero-length node name.\n");
        return 1;
    }

    // Case 3: Node name terminator exactly at the last valid byte boundary
    INIT_FDT();
    *p++ = cpu_to_fdt32(FDT_BEGIN_NODE);
    strcpy((char *)p, "123"); // 4 bytes exactly -> 4 aligned
    p += 1;
    *p++ = cpu_to_fdt32(FDT_END_NODE);
    *p++ = cpu_to_fdt32(FDT_END);
    fdt->size_dt_struct = cpu_to_fdt32((uint8_t*)p - (buffer + sizeof(struct fdt_header)));

    res = fdt_parse_discovery(buffer, &disco);
    if (res != 0) {
        printf("Test 3 Failed: Node terminator at exact boundary.\n");
        return 1;
    }

    // Case 4: Multiple NUL-separated compatible strings
    INIT_FDT();
    strcpy(str_table, "compatible");
    fdt->size_dt_strings = cpu_to_fdt32(64);

    *p++ = cpu_to_fdt32(FDT_BEGIN_NODE);
    strcpy((char *)p, "gic@0");
    p += 2; // 6 bytes -> 8 aligned

    *p++ = cpu_to_fdt32(FDT_PROP);
    char comp_data[] = "arm,cortex-a15-gic\0arm,gic-400\0";
    uint32_t comp_len = 32; // Needs to be 4-byte aligned
    *p++ = cpu_to_fdt32(sizeof(comp_data) - 1); // len should be real unaligned size for compatible
    *p++ = cpu_to_fdt32(0); // nameoff (points to "compatible")
    memset((char*)p, 0, comp_len);
    memcpy((char*)p, comp_data, sizeof(comp_data) - 1); // exclude implicit extra NUL if we want
    p += comp_len / 4;

    // Need reg for the node to be registered as an IRQ controller
    *p++ = cpu_to_fdt32(FDT_PROP);
    *p++ = cpu_to_fdt32(16); // len
    *p++ = cpu_to_fdt32(11); // nameoff (points to "reg", "compatible" is 11 bytes)
    *p++ = cpu_to_fdt32(0x00001000); // base (2 cells)
    *p++ = cpu_to_fdt32(0x00000000);
    *p++ = cpu_to_fdt32(0x00001000); // size (2 cells)
    *p++ = cpu_to_fdt32(0x00000000);
    strcpy(str_table + 11, "reg");

    *p++ = cpu_to_fdt32(FDT_END_NODE);
    *p++ = cpu_to_fdt32(FDT_END);
    fdt->size_dt_struct = cpu_to_fdt32((uint8_t*)p - (buffer + sizeof(struct fdt_header)));

    res = fdt_parse_discovery(buffer, &disco);
    if (res != 0 || disco.irq_ctrl_count == 0) {
        printf("Test 4 Failed: Multiple NUL-separated strings compatible property failed res=%d gic=%d.\n", res, disco.irq_ctrl_count);
        return 1;
    }

    // Case 5: Truncated Property header
    INIT_FDT();
    *p++ = cpu_to_fdt32(FDT_BEGIN_NODE);
    strcpy((char *)p, "node1"); // 6 bytes -> 8 aligned
    p += 2;
    *p++ = cpu_to_fdt32(FDT_PROP);
    // Not writing len and nameoff
    fdt->size_dt_struct = cpu_to_fdt32((uint8_t*)p - (buffer + sizeof(struct fdt_header)));

    res = fdt_parse_discovery(buffer, &disco);
    if (res == 0) {
        printf("Test 5 Failed: Truncated FDT_PROP header parsed.\n");
        return 1;
    }

    // Case 6: Invalid fdt_get_string offset
    INIT_FDT();
    fdt->size_dt_strings = cpu_to_fdt32(10);
    *p++ = cpu_to_fdt32(FDT_BEGIN_NODE);
    strcpy((char *)p, "node1");
    p += 2;
    *p++ = cpu_to_fdt32(FDT_PROP);
    *p++ = cpu_to_fdt32(4); // len
    *p++ = cpu_to_fdt32(100); // Invalid offset
    *p++ = cpu_to_fdt32(0); // data
    fdt->size_dt_struct = cpu_to_fdt32((uint8_t*)p - (buffer + sizeof(struct fdt_header)));

    res = fdt_parse_discovery(buffer, &disco);
    if (res == 0) {
        printf("Test 6 Failed: Invalid string offset parsed.\n");
        return 1;
    }

    printf("All advanced boundary tests passed!\n");
    return 0;
}
