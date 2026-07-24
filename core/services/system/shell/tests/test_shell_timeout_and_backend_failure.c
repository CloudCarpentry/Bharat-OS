#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "shell_dispatch.h"
#include "shell_session.h"

static int fail_uptime(uint64_t* up) { (void)up; return -1; }

int main(void) {
    shell_session_t session;
    shell_argv_t argv;
    shell_response_t r;

    shell_backend_api_t failing = *shell_default_backend();
    failing.get_uptime_ms = fail_uptime;

    shell_session_init(&session, SHELL_MODE_DEV, SHELL_CAP_DIAG);

    argv.count = 1;
    argv.tokens[0] = "uptime";
    r = shell_dispatch(&session, &failing, &argv);
    assert(r.code == SHELL_RC_BACKEND_UNAVAILABLE);

    argv.count = 2;
    argv.tokens[0] = "diag";
    argv.tokens[1] = "run";
    r = shell_dispatch(&session, shell_default_backend(), &argv);
    assert(r.code == SHELL_RC_TIMEOUT || r.code == SHELL_RC_OK);

    printf("test_shell_timeout_and_backend_failure passed\n");
    return 0;
}
