#include <bharat/bsys/backend.h>
#include <standard/assert.h>
#include <standard/stddef.h>

void bh_bsys_init_bharat_backend(void);

int main(void) {
    bh_bsys_init_bharat_backend();

    const bh_bsys_backend_ops_t *ops = bh_bsys_get_backend();
    assert(ops != NULL);

    /* Non-host backend skeleton should return -38 (SYS_ENOSYS) */
    uint32_t written = 0;
    int32_t rc = ops->write(1, "test", 4, &written);
    assert(rc == -38);

    rc = ops->read(1, NULL, 0, NULL);
    assert(rc == -38);

    rc = ops->close(1);
    assert(rc == -38);

    return 0;
}
