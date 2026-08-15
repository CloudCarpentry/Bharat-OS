import re

with open("core/kernel/src/trap/syscall_gate.c", "r") as f:
    content = f.read()

# Fix the return NULL mistake
content = content.replace("    for (;;) {\n        sched_reschedule();\n    }\n    return NULL;\n}", "    for (;;) {\n        sched_reschedule();\n    }\n}")
content = content.replace("    }\n    return NULL;\n}", "    }\n    return 0;\n}")

with open("core/kernel/src/trap/syscall_gate.c", "w") as f:
    f.write(content)
