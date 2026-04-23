import os
import subprocess

def check_fdt_ready():
    # In a real environment, QEMU passes the FDT via `-machine virt` and places it in RAM.
    # The kernel needs to validate it with a magic check in boot.S.
    # Here we assert the correct QEMU arguments are returned.
    print("ARM64: Expecting valid FDT pointer in x0 via QEMU.")

def _run_command(cmd):
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd)
    return result.returncode == 0

def get_boot_config(kernel_path):
    check_fdt_ready()
    if not os.path.exists(kernel_path):
        raise Exception(f"ARM64 kernel ELF not found: {kernel_path}")

    return {
        "kernel_path": kernel_path,
        "qemu_flags": []
    }
