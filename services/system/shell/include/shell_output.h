#ifndef BHARAT_SYSTEM_SHELL_OUTPUT_H
#define BHARAT_SYSTEM_SHELL_OUTPUT_H

#include <stddef.h>
#include "shell.h"

size_t shell_format_response(shell_output_mode_t mode,
                             const shell_response_t* response,
                             char* out,
                             size_t out_len);

#endif
