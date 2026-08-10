#include <standard/string.h>
#include <standard/assert.h>
#include <bharat/libc/stdio_min.h>
#include <bharat/libc/alloc.h>
#include <bharat/libc/build_info.h>
#include <bharat/bsys/backend.h>

void bh_bsys_init_host_backend(void);

int main(void) {
    /* Register host backend for standard I/O */
    bh_bsys_init_host_backend();

    /* 1. Test build info */
    const bh_libc_build_info_t *bi = bh_libc_get_build_info();
    bh_print_str("Consumer successfully running on BharatLibC version: ");
    bh_print_str(bi->version);
    bh_print_str("\n");

    /* 2. Test string manipulation */
    char buf[32];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, "Relocation success!", 19);
    assert(strcmp(buf, "Relocation success!") == 0);
    bh_print_str(buf);
    bh_print_str("\n");

    return 0;
}
