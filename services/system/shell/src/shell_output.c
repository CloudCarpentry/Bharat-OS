#include "shell_output.h"

#include "shell_string.h"

static size_t append_text(char* out, size_t out_len, size_t at, const char* text) {
    size_t i = 0;
    if (!text) {
        return at;
    }
    while (text[i] != '\0' && (at + 1u) < out_len) {
        out[at++] = text[i++];
    }
    return at;
}

static size_t append_u32(char* out, size_t out_len, size_t at, uint32_t value) {
    char tmp[11];
    size_t n = 0;
    do {
        tmp[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u && n < sizeof(tmp));

    while (n > 0 && (at + 1u) < out_len) {
        out[at++] = tmp[--n];
    }
    return at;
}

size_t shell_format_response(shell_output_mode_t mode,
                             const shell_response_t* response,
                             char* out,
                             size_t out_len) {
    const char* message;
    const char* payload;
    size_t at = 0;

    if (!response || !out || out_len == 0u) {
        return 0;
    }

    message = response->message ? response->message : "";
    payload = response->payload ? response->payload : "";

    if (mode == SHELL_OUTPUT_KV) {
        at = append_text(out, out_len, at, "code=");
        at = append_u32(out, out_len, at, (uint32_t)response->code);
        at = append_text(out, out_len, at, "\nmessage=");
        at = append_text(out, out_len, at, message);
        at = append_text(out, out_len, at, "\npayload=");
        at = append_text(out, out_len, at, payload);
        at = append_text(out, out_len, at, "\n");
    } else {
        at = append_text(out, out_len, at, "[");
        at = append_u32(out, out_len, at, (uint32_t)response->code);
        at = append_text(out, out_len, at, "] ");
        at = append_text(out, out_len, at, message);
        if (payload[0] != '\0') {
            at = append_text(out, out_len, at, " | ");
            at = append_text(out, out_len, at, payload);
        }
        at = append_text(out, out_len, at, "\n");
    }

    out[(at < out_len) ? at : (out_len - 1u)] = '\0';
    return at;
}
