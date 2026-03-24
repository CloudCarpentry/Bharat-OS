#ifndef NAMESVC_IPC_DISPATCH_H
#define NAMESVC_IPC_DISPATCH_H

#include <bharat/namesvc/namesvc_ipc.h>
#include <bharat/uapi/ipc/contract.h>

#ifdef __cplusplus
extern "C" {
#endif

void namesvc_ipc_handle_request(const bharat_ipc_contract_header_t *hdr, const namesvc_ipc_req_t *req, namesvc_ipc_res_t *res);

#ifdef __cplusplus
}
#endif

#endif // NAMESVC_IPC_DISPATCH_H
