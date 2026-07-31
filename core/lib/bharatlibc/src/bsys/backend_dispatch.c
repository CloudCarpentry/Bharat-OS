#include <bharat/bsys/backend.h>
#include <standard/stddef.h>

static const bh_bsys_backend_ops_t *g_backend = NULL;

void bh_bsys_register_backend(const bh_bsys_backend_ops_t *ops) {
    g_backend = ops;
}

const bh_bsys_backend_ops_t *bh_bsys_get_backend(void) {
    return g_backend;
}
