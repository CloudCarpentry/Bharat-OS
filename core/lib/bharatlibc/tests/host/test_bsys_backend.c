#include <bharat/bsys/backend.h>
#include <standard/assert.h>
#include <standard/stddef.h>

/* Declaration of host backend initializer */
void bh_bsys_init_host_backend(void);

int main(void) {
    /* Set up host backend */
    bh_bsys_init_host_backend();

    const bh_bsys_backend_ops_t *ops = bh_bsys_get_backend();
    assert(ops != NULL);
    assert(ops->write != NULL);
    assert(ops->read != NULL);

    /* Test writing */
    uint32_t written = 0;
    int32_t rc = ops->write(1, "BSys Backend Test Passed\n", 25, &written);
    assert(rc == 0);
    assert(written == 25);

    /* Test clock */
    bh_bsys_timespec_t ts = {0, 0};
    rc = ops->clock_gettime(0, &ts);
    assert(rc == 0);
    assert(ts.tv_sec != 0 || ts.tv_nsec != 0);

    return 0;
}
