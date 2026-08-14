import sys

with open("core/kernel/src/sched/sched.c", "r") as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if line.startswith("void sched_sleep_enqueue"):
        lines.insert(i+4, "  if (!sched_core_id_valid(core_id) || slot->thread.home_core_id != core_id) {\n    return;\n  }\n")
    if line.startswith("void sched_block_enqueue"):
        lines.insert(i+4, "  if (!sched_core_id_valid(core_id) || slot->thread.home_core_id != core_id) {\n    return;\n  }\n")

with open("core/kernel/src/sched/sched.c", "w") as f:
    f.writelines(lines)
