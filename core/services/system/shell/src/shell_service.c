#include "shell_service.h"
#include "shell_string.h"
#include "shell_dispatch.h"
#include "shell_output.h"
#include "shell_parser.h"
#include "shell_session.h"
#include "bharat/runtime/runtime.h"

#define CONSOLE_ENDPOINT 20

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

    response = shell_dispatch(session, backend, &argv);
    shell_format_response(session->output_mode, &response, out, out_len);
    return response.code;
}

#if !defined(SHELL_NO_MAIN)
extern const shell_backend_api_t* shell_runtime_backend(void);

int main(void) {
    shell_session_t session;
    const shell_backend_api_t* backend = shell_runtime_backend();
    char line[SHELL_MAX_INPUT_LEN];
    char out[SHELL_MAX_OUTPUT_LEN];

    bharat_runtime_log("Shell service starting");

    shell_session_init(&session, SHELL_MODE_PROD, SHELL_CAP_DIAG);

    // Welcome message to console
    const char *welcome = "Bharat-OS Shell v1.0\n# ";
    bharat_ipc_msg_header_t write_hdr = { .opcode = 2, .payload_size = (uint32_t)shell_strlen(welcome) };
    bharat_ipc_send(CONSOLE_ENDPOINT, &write_hdr, welcome);

    bharat_ipc_msg_header_t hdr;
    uint8_t payload[1024];

    while (true) {
        // Blocking receive for TTY input or future commands
        int32_t ret = bharat_ipc_recv(BHARAT_CAP_INVALID_HANDLE, &hdr, payload, sizeof(payload));
        if (ret == 0) {
            // Handle input from Console TTY (Opcode 5 in IDL is ReadSession,
            // but for async push we might define a different opcode)
            // For now, skeleton demonstration of one-shot command
            static bool once = false;
            if (!once) {
                shell_memcpy(line, "help", 5);
                shell_process_line(&session, backend, line, out, sizeof(out));

                write_hdr.payload_size = (uint32_t)shell_strlen(out);
                bharat_ipc_send(CONSOLE_ENDPOINT, &write_hdr, out);

                const char *prompt = "\n# ";
                write_hdr.payload_size = (uint32_t)shell_strlen(prompt);
                bharat_ipc_send(CONSOLE_ENDPOINT, &write_hdr, prompt);
                once = true;
            }
        }
    }
    return 0;
}
#endif
