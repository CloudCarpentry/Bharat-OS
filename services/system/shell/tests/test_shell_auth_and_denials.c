#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "shell_dispatch.h"
#include "shell_session.h"

int main(void) {
    shell_session_t session;
    shell_argv_t argv;
    const shell_backend_api_t* backend = shell_default_backend();
    shell_response_t r;
    char reboot[] = "reboot";

    shell_session_init(&session, SHELL_MODE_PROD, SHELL_CAP_NONE);

    argv.count = 1;
    argv.tokens[0] = reboot;

    r = shell_dispatch(&session, backend, &argv);
    assert(r.code == SHELL_RC_FORBIDDEN);

    r = shell_dispatch(&session, backend, &argv);
    assert(r.code == SHELL_RC_FORBIDDEN);

    r = shell_dispatch(&session, backend, &argv);
    assert(r.code == SHELL_RC_FORBIDDEN);

    r = shell_dispatch(&session, backend, &argv);
    assert(r.code == SHELL_RC_AUTH_LOCKED);

    printf("test_shell_auth_and_denials passed\n");
    return 0;
}
