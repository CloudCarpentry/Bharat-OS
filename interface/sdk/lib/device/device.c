#include <bharat/device.h>
bh_status_t bh_device_find(uint32_t c,uint32_t i,bh_device_info_t *n){(void)c;(void)i;(void)n;return BH_ERR_UNSUPPORTED;}
bh_status_t bh_device_open(uint64_t i,bh_cap_t a,bh_handle_t *d){(void)i;(void)a;(void)d;return BH_ERR_UNSUPPORTED;}
bh_status_t bh_device_query(bh_handle_t d,bh_device_info_t *i){(void)d;(void)i;return BH_ERR_UNSUPPORTED;}
bh_status_t bh_device_ioctl(bh_handle_t d,uint32_t o,const void *i,size_t z,void *u,size_t v){(void)d;(void)o;(void)i;(void)z;(void)u;(void)v;return BH_ERR_UNSUPPORTED;}
