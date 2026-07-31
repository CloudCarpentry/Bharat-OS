#include <bharat/libc/build_info.h>
#include <bharat/libc/profile.h>
#include <standard/assert.h>
#include <standard/string.h>

int main(void) {
    const bh_libc_build_info_t *bi = bh_libc_get_build_info();
    assert(bi != NULL);
    assert(bi->version != NULL);
    assert(bi->build_date != NULL);
    assert(bi->build_type != NULL);
    assert(bi->target_arch != NULL);

    const bh_libc_profile_info_t *pi = bh_libc_get_profile_info();
    assert(pi != NULL);
    assert(pi->name != NULL);
    assert(pi->memory_model != NULL);
    assert(pi->execution_profile != NULL);
    assert(pi->device_profile != NULL);
    assert(pi->personality != NULL);

    return 0;
}
