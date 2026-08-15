import shutil
import sys
import os

def check_binary(name):
    return shutil.which(name) is not None

def check_path(path):
    return os.path.exists(path)

def run(args):
    print("Bharat-OS Development Environment\n")

    def get_status(ok):
        try:
            return "✓" if ok else "✗"
        except Exception:
            return "[OK]" if ok else "[X]"

    # Compiler
    print("Compiler")
    for tool in ["clang", "lld"]:
        print(f"  {get_status(check_binary(tool))} {tool}")
    print()

    # Emulators
    print("Emulators")
    for tool in ["qemu-system-x86_64", "qemu-system-aarch64", "qemu-system-riscv64", "qemu-system-arm", "qemu-system-riscv32"]:
        print(f"  {get_status(check_binary(tool))} {tool}")
    print()

    # Tools
    print("Tools")
    for tool in ["cmake", "ninja", "python" if sys.platform == "win32" else "python3", "gdb"]:
        display_name = "python" if tool.startswith("python") else tool
        print(f"  {get_status(check_binary(tool))} {display_name}")
    print()

    # SDK
    print("SDK")
    sdk_checks = {
        "sysroot": "core/boot",
        "uapi": "interface/uapi",
        "libc": "core/lib",
    }
    for name, path in sdk_checks.items():
        print(f"  {get_status(check_path(path))} {name}")
    print()

    print("Ready.")
    return 0
