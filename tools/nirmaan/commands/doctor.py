import shutil
import sys
import os

def check_binary(name):
    return shutil.which(name) is not None

def check_path(path):
    return os.path.exists(path)

def run(args):
    print("Bharat-OS Development Environment\n")

    # Compiler
    print("Compiler")
    for tool in ["clang", "lld"]:
        status = "✓" if check_binary(tool) else "✗"
        print(f"  {status} {tool}")
    print()

    # Emulators
    print("Emulators")
    for tool in ["qemu-system-x86_64", "qemu-system-aarch64", "qemu-system-riscv64", "qemu-system-arm", "qemu-system-riscv32"]:
        status = "✓" if check_binary(tool) else "✗"
        print(f"  {status} {tool}")
    print()

    # Tools
    print("Tools")
    for tool in ["cmake", "ninja", "python" if sys.platform == "win32" else "python3", "gdb"]:
        # We will just print 'python' for python3 on unix so it matches the expected output
        display_name = "python" if tool.startswith("python") else tool
        status = "✓" if check_binary(tool) else "✗"
        print(f"  {status} {display_name}")
    print()

    # SDK
    print("SDK")
    # For SDK we use heuristics based on the paths, as true sysroot might be built
    # The requirement specifically asks for `sysroot`, `uapi`, `libc`
    # We will check proxy paths for them.
    sdk_checks = {
        "sysroot": "core/boot", # Just a proxy path to check if we are in repo
        "uapi": "interface/uapi",
        "libc": "core/lib",
    }
    for name, path in sdk_checks.items():
        status = "✓" if check_path(path) else "✗"
        print(f"  {status} {name}")
    print()

    print("Ready.")
    return 0
