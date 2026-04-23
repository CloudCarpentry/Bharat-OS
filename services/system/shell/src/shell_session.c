#include "shell_session.h"

void shell_session_init(shell_session_t* session, shell_mode_t mode, uint32_t caps_mask) {
    if (!session) {
        return;
    }
    session->mode = mode;
    session->caps_mask = caps_mask;
    session->output_mode = SHELL_OUTPUT_TEXT;
    session->failed_auth_count = 0;
    session->locked = false;
}
