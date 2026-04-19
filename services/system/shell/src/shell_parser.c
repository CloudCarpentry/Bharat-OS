#include "shell_parser.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

static bool shell_is_valid_text_char(unsigned char c) {
    if (c == '\t' || c == ' ') {
        return true;
    }
    if (c >= 32u && c <= 126u) {
        return true;
    }
    return false;
}

shell_status_code_t shell_parse_line(char* line, shell_argv_t* out_argv) {
    size_t i = 0;

    if (!line || !out_argv) {
        return SHELL_RC_INVALID_ARG;
    }

    out_argv->count = 0;

    for (i = 0; line[i] != '\0'; ++i) {
        if (i >= SHELL_MAX_INPUT_LEN - 1u) {
            return SHELL_RC_PARSE_ERROR;
        }
        if (!shell_is_valid_text_char((unsigned char)line[i])) {
            return SHELL_RC_PARSE_ERROR;
        }
    }

    i = 0;
    while (line[i] != '\0') {
        while (line[i] == ' ' || line[i] == '\t') {
            ++i;
        }
        if (line[i] == '\0') {
            break;
        }
        if (out_argv->count >= SHELL_MAX_TOKENS) {
            return SHELL_RC_PARSE_ERROR;
        }

        out_argv->tokens[out_argv->count++] = &line[i];

        while (line[i] != '\0' && line[i] != ' ' && line[i] != '\t') {
            ++i;
        }
        if (line[i] == '\0') {
            break;
        }
        line[i++] = '\0';
    }

    return SHELL_RC_OK;
}
