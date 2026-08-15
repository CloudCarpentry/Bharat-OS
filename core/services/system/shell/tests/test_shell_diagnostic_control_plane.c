#include "shell_dispatch.h"
#include "shell_parser.h"
#include "shell_session.h"

#include <assert.h>
#include <string.h>

static bh_shell_diag_request_v1_t observed;

static int diagnostic_service(const bh_shell_diag_request_v1_t *request,
                              char *out, size_t out_len) {
  assert(request != NULL);
  assert(out_len >= sizeof("source=service"));
  observed = *request;
  memcpy(out, "source=service", sizeof("source=service"));
  return 0;
}

int main(void) {
  shell_session_t session;
  shell_backend_api_t backend = {0};
  shell_argv_t argv;
  shell_response_t response;
  char line[] = "service status console";
  char unavailable_line[] = "cpuinfo";
  char timer_test_line[] = "timer test";

  shell_session_init(&session, SHELL_MODE_DEV, SHELL_CAP_NONE);
  backend.diag_query = diagnostic_service;
  assert(shell_parse_line(line, &argv) == 0);
  response = shell_dispatch(&session, &backend, &argv);
  assert(response.code == SHELL_RC_OK);
  assert(strcmp(response.payload, "source=service") == 0);
  assert(observed.abi_version == BH_SHELL_DIAG_ABI_VERSION);
  assert(observed.struct_size == sizeof(observed));
  assert(observed.query == BH_SHELL_DIAG_SERVICE_STATUS);
  assert(observed.argument_length == strlen("console"));
  assert(strcmp(observed.argument, "console") == 0);

  assert(shell_parse_line(timer_test_line, &argv) == 0);
  response = shell_dispatch(&session, &backend, &argv);
  assert(response.code == SHELL_RC_FORBIDDEN);
  session.caps_mask = SHELL_CAP_DIAG;
  response = shell_dispatch(&session, &backend, &argv);
  assert(response.code == SHELL_RC_OK);
  assert(observed.query == BH_SHELL_DIAG_TIMER_TEST);

  backend.diag_query = NULL;
  assert(shell_parse_line(unavailable_line, &argv) == 0);
  response = shell_dispatch(&session, &backend, &argv);
  assert(response.code == SHELL_RC_BACKEND_UNAVAILABLE);
  return 0;
}
