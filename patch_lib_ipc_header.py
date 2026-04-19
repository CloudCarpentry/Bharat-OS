import sys

with open("lib/include/ipc_user.h", "r") as f:
    content = f.read()

content = "long bharat_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);\n" + content

with open("lib/include/ipc_user.h", "w") as f:
    f.write(content)
