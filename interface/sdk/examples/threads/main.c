#include <bharat/os.h>
#include <bharat/thread.h>
static void worker(void *context) { (void)context; }
int main(void) { bh_handle_t thread; bh_status_t status = bh_thread_create(worker, 0, 0, &thread); if (status == BH_ERR_UNSUPPORTED) return bh_log("threads: unavailable on hosted v0.1 backend\n") != BH_OK; return status != BH_OK; }
