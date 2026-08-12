#include <bharat/ipc.h>
#include <bharat/os.h>
int main(void) { bh_endpoint_t endpoint; bh_status_t status = bh_endpoint_open("demo.ping", &endpoint); if (status == BH_ERR_UNSUPPORTED) return bh_log("ipc: unavailable on hosted v0.1 backend\n") != BH_OK; return status != BH_OK; }
