#ifndef BHARAT_SYSTEM_SHELL_DISPATCH_H
#define BHARAT_SYSTEM_SHELL_DISPATCH_H

#include "shell.h"
#include "shell_backend.h"

shell_response_t shell_dispatch(shell_session_t* session,
                                const shell_backend_api_t* backend,
                                const shell_argv_t* argv);

#endif
