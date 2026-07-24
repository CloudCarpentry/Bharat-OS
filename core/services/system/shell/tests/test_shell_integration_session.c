#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "shell_service.h"
#include "shell_session.h"

int main(void) {
    shell_session_t session;
    char out[SHELL_MAX_OUTPUT_LEN];
    const shell_backend_api_t* backend = shell_default_backend();

    shell_session_init(&session, SHELL_MODE_DEV, SHELL_CAP_REBOOT | SHELL_CAP_DIAG);

    char help[] = "help";
    assert(shell_process_line(&session, backend, help, out, sizeof(out)) == SHELL_RC_OK);
    assert(strstr(out, "commands") != NULL);

    char mode[] = "mode kv";
    assert(shell_process_line(&session, backend, mode, out, sizeof(out)) == SHELL_RC_OK);
    assert(strstr(out, "code=") != NULL);

    char svc[] = "svc status console";
    assert(shell_process_line(&session, backend, svc, out, sizeof(out)) == SHELL_RC_OK);
    assert(strstr(out, "service=console") != NULL);

    printf("test_shell_integration_session passed\n");
    return 0;
}
