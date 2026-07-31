#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "shell_dispatch.h"
#include "shell_session.h"

// 1. Mock functions for Successful diagnostic and other cases

static int mock_uptime_zero(uint64_t* up) {
    if (up) *up = 1000;
    return 0;
}

static int mock_diag_success(char* out, size_t out_len) {
    if (out && out_len > 15) {
        strcpy(out, "diag_ok");
    }
    return 0;
}

// Global flag to track if diag_run callback was called
static int g_diag_called = 0;
static int mock_diag_track(char* out, size_t out_len) {
    g_diag_called = 1;
    if (out && out_len > 15) {
        strcpy(out, "diag_ok");
    }
    return 0;
}

static int mock_diag_unsupported(char* out, size_t out_len) {
    (void)out; (void)out_len;
    return -1; // returns non-zero / unsupported
}

// 2. Mock functions for Deterministic timeout
static uint64_t g_fake_uptime = 100;
static int mock_uptime_incrementing(uint64_t* up) {
    if (up) {
        *up = g_fake_uptime;
        g_fake_uptime += 5; // increment by 5ms on each call (exceeds timeout_ms = 1)
    }
    return 0;
}

static int fail_uptime(uint64_t* up) { (void)up; return -1; }

int main(void) {
    shell_session_t session;
    shell_argv_t argv;
    shell_response_t r;

    // --- BASELINE TEST 1: UPTIME BACKEND UNAVAILABLE ---
    shell_backend_api_t failing = *shell_default_backend();
    failing.get_uptime_ms = fail_uptime;

    shell_session_init(&session, SHELL_MODE_DEV, SHELL_CAP_DIAG);

    argv.count = 1;
    argv.tokens[0] = "uptime";
    r = shell_dispatch(&session, &failing, &argv);
    assert(r.code == SHELL_RC_BACKEND_UNAVAILABLE);


    // --- TEST 1: SUCCESSFUL DIAGNOSTIC ---
    // diagnostic callback returns success;
    // monotonic backend returns no elapsed-time increase (or under limit);
    // expect SHELL_RC_OK.
    shell_backend_api_t success_backend = *shell_default_backend();
    success_backend.get_uptime_ms = mock_uptime_zero;
    success_backend.diag_run = mock_diag_success;

    shell_session_init(&session, SHELL_MODE_DEV, SHELL_CAP_DIAG);

    argv.count = 2;
    argv.tokens[0] = "diag";
    argv.tokens[1] = "run";
    r = shell_dispatch(&session, &success_backend, &argv);
    assert(r.code == SHELL_RC_OK);
    assert(r.payload != NULL);
    assert(strcmp(r.payload, "diag_ok") == 0);


    // --- TEST 2: BACKEND UNAVAILABLE (unsupported status) ---
    // callback is unsupported (returns non-zero);
    // expect SHELL_RC_BACKEND_UNAVAILABLE.
    shell_backend_api_t unsupported_backend = *shell_default_backend();
    unsupported_backend.get_uptime_ms = mock_uptime_zero;
    unsupported_backend.diag_run = mock_diag_unsupported;

    shell_session_init(&session, SHELL_MODE_DEV, SHELL_CAP_DIAG);

    argv.count = 2;
    argv.tokens[0] = "diag";
    argv.tokens[1] = "run";
    r = shell_dispatch(&session, &unsupported_backend, &argv);
    assert(r.code == SHELL_RC_BACKEND_UNAVAILABLE);


    // --- TEST 2B: BACKEND UNAVAILABLE (callback is absent) ---
    // callback is NULL;
    // expect SHELL_RC_BACKEND_UNAVAILABLE.
    shell_backend_api_t null_backend = *shell_default_backend();
    null_backend.get_uptime_ms = mock_uptime_zero;
    null_backend.diag_run = NULL;

    shell_session_init(&session, SHELL_MODE_DEV, SHELL_CAP_DIAG);

    argv.count = 2;
    argv.tokens[0] = "diag";
    argv.tokens[1] = "run";
    r = shell_dispatch(&session, &null_backend, &argv);
    assert(r.code == SHELL_RC_BACKEND_UNAVAILABLE);


    // --- TEST 3: DETERMINISTIC TIMEOUT ---
    // diagnostic callback returns success;
    // injected clock returns elapsed-time increase exceeding timeout_ms = 1;
    // expect SHELL_RC_TIMEOUT.
    shell_backend_api_t timeout_backend = *shell_default_backend();
    g_fake_uptime = 100;
    timeout_backend.get_uptime_ms = mock_uptime_incrementing;
    timeout_backend.diag_run = mock_diag_success;

    shell_session_init(&session, SHELL_MODE_DEV, SHELL_CAP_DIAG);

    argv.count = 2;
    argv.tokens[0] = "diag";
    argv.tokens[1] = "run";
    r = shell_dispatch(&session, &timeout_backend, &argv);
    assert(r.code == SHELL_RC_TIMEOUT);


    // --- TEST 4: ACCESS DENIAL ---
    // session lacks SHELL_CAP_DIAG;
    // diagnostic callback must NOT be invoked.
    shell_backend_api_t deny_backend = *shell_default_backend();
    g_diag_called = 0;
    deny_backend.get_uptime_ms = mock_uptime_zero;
    deny_backend.diag_run = mock_diag_track;

    shell_session_init(&session, SHELL_MODE_DEV, SHELL_CAP_NONE); // No SHELL_CAP_DIAG capability

    argv.count = 2;
    argv.tokens[0] = "diag";
    argv.tokens[1] = "run";
    r = shell_dispatch(&session, &deny_backend, &argv);
    assert(r.code == SHELL_RC_FORBIDDEN);
    assert(g_diag_called == 0); // Must not have been called!


    // --- TEST 5: PRODUCTION RESTRICTION ---
    // confirm `diag run` remains denied where `allowed_in_prod` policy requires denial.
    shell_backend_api_t prod_backend = *shell_default_backend();
    g_diag_called = 0;
    prod_backend.get_uptime_ms = mock_uptime_zero;
    prod_backend.diag_run = mock_diag_track;

    shell_session_init(&session, SHELL_MODE_PROD, SHELL_CAP_DIAG); // CAP_DIAG present, but in production mode!

    argv.count = 2;
    argv.tokens[0] = "diag";
    argv.tokens[1] = "run";
    r = shell_dispatch(&session, &prod_backend, &argv);
    assert(r.code == SHELL_RC_FORBIDDEN);
    assert(g_diag_called == 0); // Must not have been called in production mode!


    printf("test_shell_timeout_and_backend_failure passed\n");
    return 0;
}
