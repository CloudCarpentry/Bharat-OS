#ifndef BHARAT_MSG_ERRORS_H
#define BHARAT_MSG_ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Protocol Error Codes
// ============================================================================

#define BHARAT_MSG_OK                     0
#define BHARAT_MSG_ERR_MALFORMED_HEADER  -1
#define BHARAT_MSG_ERR_MALFORMED_PAYLOAD -2
#define BHARAT_MSG_ERR_UNSUPPORTED_VER   -3
#define BHARAT_MSG_ERR_UNKNOWN_SERVICE   -4
#define BHARAT_MSG_ERR_UNKNOWN_OPCODE    -5
#define BHARAT_MSG_ERR_UNAUTHORIZED      -6
#define BHARAT_MSG_ERR_TRANSPORT_FAIL    -7
#define BHARAT_MSG_ERR_TIMEOUT           -8
#define BHARAT_MSG_ERR_TOO_LARGE         -9
#define BHARAT_MSG_ERR_UNSUPPORTED_FEAT  -10
#define BHARAT_MSG_ERR_INVALID_CAP       -11
#define BHARAT_MSG_ERR_CAP_REVOKED       -12
#define BHARAT_MSG_ERR_INCOMPAT_TRANS    -13
#define BHARAT_MSG_ERR_TRUNCATED         -14
#define BHARAT_MSG_ERR_BUFFER_OVERFLOW   -15

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MSG_ERRORS_H
