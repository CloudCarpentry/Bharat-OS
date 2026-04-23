import os
import subprocess

def run_command(cmd):
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd)
    if result.returncode != 0:
        raise Exception(f"Command failed with exit code {result.returncode}")

def check_multiboot_header(kernel_path):
    print(f"Validating Multiboot header in {kernel_path}...")
    try:
        # We search for Multiboot 1 magic 0x1badb002 or Multiboot 2 magic 0xe85250d6
        cmd = ['llvm-objdump', '-s', kernel_path]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            out = result.stdout.lower()
            if "02b0ad1b" in out or "d65052e8" in out:
                print("x86_64: Multiboot compliant ELF verified.")
            else:
                raise Exception("Multiboot header magic number not found in objdump output.")
    except Exception as e:
        print(f"Warning: Failed to validate multiboot header: {e}")
        raise e


def get_boot_config(kernel_path):
    print("x86_64: Converting to 32-bit ELF (Multiboot compatibility)")
    kernel32_path = kernel_path.replace('kernel.elf', 'kernel32.elf')
    cmd = ['llvm-objcopy', '-I', 'elf64-x86-64', '-O', 'elf32-i386', kernel_path, kernel32_path]
    run_command(cmd)

    check_multiboot_header(kernel32_path)

    return {
        "kernel_path": kernel32_path,
        "qemu_flags": []
    }
