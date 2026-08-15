#include <bharat/process.h>
bh_status_t bh_process_spawn(const char *p, const char *const a[], const bh_process_options_t *o, bh_handle_t *h) { (void)p;(void)a;(void)o;(void)h; return BH_ERR_UNSUPPORTED; }
bh_status_t bh_process_wait(bh_handle_t p, uint64_t t, int32_t *s) { (void)p;(void)t;(void)s; return BH_ERR_UNSUPPORTED; }
bh_status_t bh_thread_create(bh_thread_entry_t e, void *c, const bh_thread_options_t *o, bh_handle_t *t) { (void)e;(void)c;(void)o;(void)t; return BH_ERR_UNSUPPORTED; }
