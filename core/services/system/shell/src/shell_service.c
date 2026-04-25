#include "shell_service.h"

#include "shell_string.h"

#include "shell_dispatch.h"
#include "shell_output.h"
#include "shell_parser.h"
#include "shell_session.h"

#if defined(BHARAT_HOST_TEST) && !defined(SHELL_NO_MAIN)
#include <stdio.h>

static void trim_line_end(char* line) {
    size_t len;

    if (!line) {
        return;
    }

    len = shell_strlen(line);
    while (len > 0u && (line[len - 1u] == '\n' || line[len - 1u] == '\r')) {
        line[len - 1u] = '\0';
        --len;
    }
}
#endif

shell_status_code_t shell_process_line(shell_session_t* session,
                                       const shell_backend_api_t* backend,
                                       char* line,
                                       char* out,
                                       size_t out_len) {
    shell_argv_t argv;
    shell_response_t response;
    shell_status_code_t parse_rc;

    if (!session || !line || !out || out_len == 0u) {
        return SHELL_RC_INVALID_ARG;
    }

    parse_rc = shell_parse_line(line, &argv);
    if (parse_rc != SHELL_RC_OK) {
        response.code = parse_rc;
        response.message = "parse error";
        response.payload = NULL;
        shell_format_response(session->output_mode, &response, out, out_len);
        return parse_rc;
    }

    if (argv.count == 0u) {
        response.code = SHELL_RC_OK;
        response.message = "";
        response.payload = "";
        shell_format_response(session->output_mode, &response, out, out_len);
        return SHELL_RC_OK;
    }

    if (shell_strcmp(argv.tokens[0], "mode") == 0 && argv.count >= 2u) {
        if (shell_strcmp(argv.tokens[1], "kv") == 0) {
            session->output_mode = SHELL_OUTPUT_KV;
        } else {
            session->output_mode = SHELL_OUTPUT_TEXT;
        }
        response.code = SHELL_RC_OK;
        response.message = "output mode updated";
        response.payload = NULL;
        shell_format_response(session->output_mode, &response, out, out_len);
        return SHELL_RC_OK;
    }

    response = shell_dispatch(session, backend, &argv);
    shell_format_response(session->output_mode, &response, out, out_len);
    return response.code;
}

#if defined(BHARAT_HOST_TEST) && !defined(SHELL_NO_MAIN)
int main(void) {
    shell_session_t session;
    const shell_backend_api_t* backend = shell_default_backend();
    char line[SHELL_MAX_INPUT_LEN];
    char out[SHELL_MAX_OUTPUT_LEN];

    shell_session_init(&session,
                       SHELL_MODE_DEV,
                       SHELL_CAP_DIAG | SHELL_CAP_REBOOT | SHELL_CAP_SVC_WRITE | SHELL_CAP_FACTORY);

    while (fgets(line, sizeof(line), stdin) != NULL) {
        trim_line_end(line);
        (void)shell_process_line(&session, backend, line, out, sizeof(out));
        puts(out);
        fflush(stdout);
    }

    return 0;
}
#elif !defined(SHELL_NO_MAIN)
int main(void) {
    shell_session_t session;
    const shell_backend_api_t* backend = shell_default_backend();
    char out[SHELL_MAX_OUTPUT_LEN];

    shell_session_init(&session,
                       SHELL_MODE_DEV,
                       SHELL_CAP_DIAG | SHELL_CAP_REBOOT | SHELL_CAP_SVC_WRITE | SHELL_CAP_FACTORY);

    for (;;) {
        shell_response_t idle = {
            .code = SHELL_RC_OK,
            .message = "shell service idle",
            .payload = NULL,
        };
        shell_format_response(session.output_mode, &idle, out, sizeof(out));
        (void)backend;
        (void)out;
    }
}
#endif
