#ifndef BHARAT_SYSTEM_SHELL_SERVICE_H
#define BHARAT_SYSTEM_SHELL_SERVICE_H

#include <stddef.h>

#include "shell.h"
#include "shell_backend.h"

shell_status_code_t shell_process_line(shell_session_t* session,
                                       const shell_backend_api_t* backend,
                                       char* line,
                                       char* out,
                                       size_t out_len);

#endif
