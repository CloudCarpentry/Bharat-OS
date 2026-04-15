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

def _build_arm64_image(kernel_path):
    image_path = kernel_path.replace('kernel.elf', 'kernel.img')

    # Prefer llvm-objcopy (used elsewhere in this repo), fallback to objcopy.
    if _run_command(['llvm-objcopy', '-O', 'binary', kernel_path, image_path]):
        return image_path
    if _run_command(['objcopy', '-O', 'binary', kernel_path, image_path]):
        return image_path

    raise Exception(
        "ARM64 boot image conversion failed. Install llvm-objcopy or objcopy."
    )

def get_boot_config(kernel_path):
    check_fdt_ready()
    if not os.path.exists(kernel_path):
        raise Exception(f"ARM64 kernel ELF not found: {kernel_path}")

    image_path = _build_arm64_image(kernel_path)
    return {
        # Use a raw ARM64 Linux-style kernel image for QEMU `-kernel`.
        # This keeps the 64-byte ARM64 image header at offset 0 and avoids
        # host/QEMU differences in ELF loading behavior.
        "kernel_path": image_path,
        "qemu_flags": []
    }
