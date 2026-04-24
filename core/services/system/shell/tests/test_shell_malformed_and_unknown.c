#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "shell_service.h"
#include "shell_session.h"

int main(void) {
    shell_session_t session;
    char out[SHELL_MAX_OUTPUT_LEN];
    const shell_backend_api_t* backend = shell_default_backend();

    shell_session_init(&session, SHELL_MODE_DEV, SHELL_CAP_NONE);

    char unknown[] = "notacommand";
    assert(shell_process_line(&session, backend, unknown, out, sizeof(out)) == SHELL_RC_UNKNOWN_COMMAND);

    char malformed[] = {0x7f,'b','a','d','\0'};
    assert(shell_process_line(&session, backend, malformed, out, sizeof(out)) == SHELL_RC_PARSE_ERROR);

    printf("test_shell_malformed_and_unknown passed\n");
    return 0;
}
