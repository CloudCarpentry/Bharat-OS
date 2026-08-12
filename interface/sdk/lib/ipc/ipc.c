#include <bharat/ipc.h>
bh_status_t bh_endpoint_open(const char *n,bh_endpoint_t *e){(void)n;(void)e;return BH_ERR_UNSUPPORTED;}
bh_status_t bh_send(bh_endpoint_t e,const bh_message_t *m,uint64_t t){(void)e;(void)m;(void)t;return BH_ERR_UNSUPPORTED;}
bh_status_t bh_recv(bh_endpoint_t e,bh_message_t *m,uint64_t t){(void)e;(void)m;(void)t;return BH_ERR_UNSUPPORTED;}
bh_status_t bh_call(bh_endpoint_t e,const bh_message_t *q,bh_message_t *r,uint64_t t){(void)e;(void)q;(void)r;(void)t;return BH_ERR_UNSUPPORTED;}
