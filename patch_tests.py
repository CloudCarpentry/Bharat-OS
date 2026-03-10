import os

with open("tests/CMakeLists.txt", "r") as f:
    content = f.read()

content = content.replace("    ../kernel/src/sched.c\n", "    ../kernel/src/sched.c\n    ../kernel/src/sched_deg.c\n")

with open("tests/CMakeLists.txt", "w") as f:
    f.write(content)
