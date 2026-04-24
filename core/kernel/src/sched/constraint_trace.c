#include <debug/constraint_trace.h>
#include <console/console_core.h>

void trace_sched_decision(int tid, int old_cpu,
                         int new_cpu, int reason)
{
    (void)tid;
    (void)old_cpu;
    (void)new_cpu;
    (void)reason;
    // MVP trace logic. Since printf is not available, we use console_write_raw.
    // In the future this should use a proper tracing subsystem.
    const char *msg = "[SCHED] Constraint-aware scheduling decision made.\n";
    console_write_raw(msg, 51); // 51 is the length of the string
}
