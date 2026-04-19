import sys

with open("lib/ipc/src/ipc.c", "r") as f:
    content = f.read()

content = content.replace("#include <string.h>", "#include <lib/base/string.h>")

with open("lib/ipc/src/ipc.c", "w") as f:
    f.write(content)
