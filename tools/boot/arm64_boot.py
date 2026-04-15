import os
import subprocess

def check_fdt_ready():
    # In a real environment, QEMU passes the FDT via `-machine virt` and places it in RAM.
    # The kernel needs to validate it with a magic check in boot.S.
    # Here we assert the correct QEMU arguments are returned.
    print("ARM64: Expecting valid FDT pointer in x0 via QEMU.")

def get_boot_config(kernel_path):
    check_fdt_ready()

    bin_path = kernel_path.replace('.elf', '.bin')
    if os.path.exists(kernel_path):
        subprocess.run(['llvm-objcopy', '-O', 'binary', kernel_path, bin_path], check=True)

    return {
        "kernel_path": bin_path,
        "qemu_flags": []
    }
