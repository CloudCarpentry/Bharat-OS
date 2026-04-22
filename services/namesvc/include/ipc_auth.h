#ifndef NAMESVC_IPC_AUTH_H
#define NAMESVC_IPC_AUTH_H

#include <stdint.h>
#include <bharat/cap/cap.h>

#ifdef __cplusplus
extern "C" {
#endif

int namesvc_authorize(uint32_t opcode, bharat_cap_handle_t caller_cap);

#ifdef __cplusplus
}
#endif

#endif // NAMESVC_IPC_AUTH_H
