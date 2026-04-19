import os
import subprocess
from pathlib import Path


def apply_multiboot_elf_fix(input_path: Path, output_path: Path):
    """
    Converts a 64-bit ELF container to a 32-bit ELF container for QEMU/Multiboot compliance.
    """
    if not input_path.is_file():
        print(f"[Warning] Input ELF file not found for transform: {input_path}")
        return

    output_path.parent.mkdir(parents=True, exist_ok=True)

    # Note: We use specific targets for x86_64 conversion
    objcopy_cmd = os.environ.get("OBJCOPY", "llvm-objcopy")
    cmd = [
        objcopy_cmd,
        "-I", "elf64-x86-64",
        "-O", "elf32-i386",
        str(input_path),
        str(output_path)
    ]

    try:
        subprocess.run(cmd, check=True, capture_output=True)
        print(f"[Package] Successfully converted 64-bit ELF to 32-bit Multiboot container: {output_path}")
    except subprocess.CalledProcessError as e:
        print(f"[{objcopy_cmd}] failed during multiboot fix: {e.stderr.decode().strip()}")
        raise e
