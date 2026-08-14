import sys

with open("core/kernel/src/sched/sched.c", "r") as f:
    lines = f.readlines()

out = []
in_reap = False

for i, line in enumerate(lines):
    if line.startswith("int sched_enqueue_reap(thread_slot_t *slot) {"):
        in_reap = True
        out.append(line)
        continue

    if in_reap:
        if "uint32_t core_id = sched_clamp_core(slot->creation_core_id);" in line:
            pass # skip
        elif "uint32_t core_id = sched_current_core_or_panic();" in line:
            pass # skip
        elif "sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;" in line:
            pass # skip
        elif "spin_lock(&rq->lock);" in line:
            out.append("  uint32_t home_core = slot->thread.home_core_id;\n")
            out.append("  if (!sched_core_id_valid(home_core)) {\n")
            out.append("    return -1;\n")
            out.append("  }\n")
            out.append("  sched_rq_t *rq = &g_cpu_locals[home_core].runqueue;\n\n")
            out.append(line)
        elif "return 0;" in line and "}" in lines[i+1]:
            out.append(line)
            in_reap = False
        else:
            out.append(line)
    else:
        out.append(line)

with open("core/kernel/src/sched/sched.c", "w") as f:
    f.writelines(out)
