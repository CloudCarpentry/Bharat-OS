#include "shell_output.h"

#include <stdio.h>

size_t shell_format_response(shell_output_mode_t mode,
                             const shell_response_t* response,
                             char* out,
                             size_t out_len) {
    int written;
    const char* message;
    const char* payload;

    if (!response || !out || out_len == 0u) {
        return 0;
    }

    message = response->message ? response->message : "";
    payload = response->payload ? response->payload : "";

    if (mode == SHELL_OUTPUT_KV) {
        written = snprintf(out, out_len,
                           "code=%u\nmessage=%s\npayload=%s\n",
                           (unsigned)response->code,
                           message,
                           payload);
    } else {
        written = snprintf(out, out_len,
                           "[%u] %s%s%s\n",
                           (unsigned)response->code,
                           message,
                           payload[0] ? " | " : "",
                           payload);
    }

    if (written < 0) {
        return 0;
    }
    if ((size_t)written >= out_len) {
        return out_len - 1u;
    }
    return (size_t)written;
}
