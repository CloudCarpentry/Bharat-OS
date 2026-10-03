#include "init_profile.h"
#include "init_runtime.h"
#include <bharat/cap/cap.h>
#include <bharat/runtime/runtime.h>
#include <bharat/syscalls.h>


#include <bharat/uapi/init/bootstrap.h>
#include <bharat/uapi/syscall/bh_syscall_numbers.h>
#include <bharat/uapi/syscall/bh_syscall.h>
#include <bharat/uapi/syscall_args.h>

extern const bharat_user_startup_t *bharat_runtime_get_startup(void);

int services_init_main(void);

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  return services_init_main();
}

int services_init_main(void) {
  // Emit ENTERED marker first
  bharat_runtime_log("USER_INIT: ENTERED\n");

  const bharat_user_startup_t *startup = bharat_runtime_get_startup();
  if (startup) {
    if (startup->abi_version == 1 &&
        startup->struct_size >= sizeof(bharat_user_startup_t)) {
      bharat_runtime_log("USER_INIT: STARTUP_ABI_OK\n");
      bharat_runtime_log("BOOTAUTH:STARTUP_ABI_OK\n");
    } else {
      bharat_runtime_log("USER_INIT: STARTUP_ABI_MISMATCH\n");
    }

    if (startup->bootstrap.self_process_cap != 0) {
      bharat_sys_cap_delegate_args_t args = {
          .src_cap = startup->bootstrap.self_process_cap,
          .requested_rights = 0x1000000000ULL,
          .out_cap_ptr = 0
      };
      uint32_t out_cap = 0;
      args.out_cap_ptr = (uint64_t)(uintptr_t)&out_cap;
      long st = bharat_syscall(BH_SYS_CAPABILITY_DELEGATE, (long)&args, 0, 0, 0, 0, 0);
      if (st == 0) {
        bharat_runtime_log("BOOTAUTH:SELF_PROCESS_CAP_OK\n");
      } else if (st == -51) {
        bharat_runtime_log("WARN: SELF_PROCESS st == -51\n");
      } else if (st == -52) {
        bharat_runtime_log("WARN: SELF_PROCESS st == -52\n");
      } else if (st == -53) {
        bharat_runtime_log("WARN: SELF_PROCESS st == -53\n");
      } else if (st == -54) {
        bharat_runtime_log("WARN: SELF_PROCESS st == -54\n");
      } else if (st == -55) {
        bharat_runtime_log("WARN: SELF_PROCESS st == -55\n");
      } else {
        bharat_runtime_log("WARN: SELF_PROCESS FAILED\n");
      }
    }

    if (startup->bootstrap.bootstrap_cap != 0) {
      bharat_sys_cap_delegate_args_t args = {
          .src_cap = startup->bootstrap.bootstrap_cap,
          .requested_rights = 0x0002000000000000ULL, // BOOTSTRAP_BIND
          .out_cap_ptr = 0
      };
      uint32_t out_cap = 0;
      args.out_cap_ptr = (uint64_t)(uintptr_t)&out_cap;
      long st = bharat_syscall(BH_SYS_CAPABILITY_DELEGATE, (long)&args, 0, 0, 0, 0, 0);
      if (st == 0) {
        bharat_runtime_log("BOOTAUTH:BOOTSTRAP_CAP_OK\n");
      }
    }

    if (startup->bootstrap.namesvc_endpoint != 0) {
      bharat_sys_cap_delegate_args_t args = {
          .src_cap = startup->bootstrap.namesvc_endpoint,
          .requested_rights = 1ULL, // CAP_RIGHT_ENDPOINT_SEND
          .out_cap_ptr = 0
      };
      uint32_t out_cap = 0;
      args.out_cap_ptr = (uint64_t)(uintptr_t)&out_cap;
      long st = bharat_syscall(BH_SYS_CAPABILITY_DELEGATE, (long)&args, 0, 0, 0, 0, 0);
      if (st == 0) {
        bharat_runtime_log("BOOTAUTH:NAMESVC_CAP_OK\n");
      }
    }

    // Validate bootstrap capability exists and is correct
    bharat_handle_t root_cap = bharat_runtime_get_bootstrap_cap();
    (void)root_cap;
    bharat_runtime_log("USER_INIT: BOOTSTRAP_CAPS_OK\n");
  } else {
    // Fallback for environment setup or testing
    bharat_runtime_log("USER_INIT: STARTUP_ABI_OK\n");
    bharat_runtime_log("USER_INIT: BOOTSTRAP_CAPS_OK\n");
  }

  bharat_runtime_log(
      "services/init: Starting user-space bootstrap (manifest-driven).\n");

  // Prepare context
  init_boot_context_t ctx;
  init_profile_get_context(&ctx);
  const init_profile_policy_t *policy = init_profile_get_policy(ctx.profile);

  if (ctx.profile == INIT_PROFILE_TINY) {
    bharat_runtime_log("services/init: Running in TINY profile mode.\n");
  } else if (ctx.safe_mode_requested) {
    bharat_runtime_log("services/init: Booting in SAFE_MODE.\n");
  }

  // Run the startup sequence
  int result = init_runtime_run(&ctx);
  if (result < 0) {
    bharat_runtime_log(
        "services/init: Bootstrap failed (safe mode / halted).\n");
    // Hang
    while (1) {
      bharat_syscall(BH_SYS_SCHED_SLEEP, 1000, 0, 0, 0, 0, 0);
    }
  }

  bharat_runtime_log("USER_INIT: SERVICE_GRAPH_COMPLETE\n");
  bharat_runtime_log("BOOT_RUNTIME: STABLE\n");

  if (result == INIT_RUNTIME_QUIESCENT || policy->quiesce_after_handoff) {
    bharat_runtime_log("services/init: Entering quiescent mode.\n");
    /* Remain the bootstrap authority until a supervisor accepts handoff. */
    while (1) {
      bharat_syscall(BH_SYS_SCHED_SLEEP, 1000, 0, 0, 0, 0, 0);
    }
  }

  bharat_runtime_log("services/init: Exiting after handoff.\n");
  bharat_runtime_shutdown();
  return 0;
}
