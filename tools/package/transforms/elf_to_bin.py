import os
import subprocess
from pathlib import Path
from shutil import which


def _resolve_objcopy() -> str:
    env_objcopy = os.environ.get("OBJCOPY")
    if env_objcopy:
        return env_objcopy

    candidates = ("llvm-objcopy", "llvm-objcopy-20", "llvm-objcopy-19", "objcopy")
    for candidate in candidates:
        if not which(candidate):
            continue
        try:
            subprocess.run(
                [candidate, "--version"],
                check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            return candidate
        except Exception:
            continue

    return "llvm-objcopy"


def apply_elf_to_bin(input_path: Path, output_path: Path):
    """
    Transforms an ELF artifact into a raw binary artifact.
    """
    if not input_path.is_file():
        # In a dry-run or if configure-only was used, this file won't exist.
        # But we still shouldn't fail if we're just checking logic.
        print(f"[Warning] Input ELF file not found for transform: {input_path}")
        return

    output_path.parent.mkdir(parents=True, exist_ok=True)

    objcopy_cmd = _resolve_objcopy()
    cmd = [objcopy_cmd, "-O", "binary", str(input_path), str(output_path)]

    try:
        subprocess.run(cmd, check=True, capture_output=True)
    except subprocess.CalledProcessError as e:
        if objcopy_cmd == "llvm-objcopy":
            print(f"[{objcopy_cmd}] failed: {e.stderr.decode().strip()}. Falling back to standard objcopy...")
            objcopy_cmd = "objcopy"
            cmd = [objcopy_cmd, "-O", "binary", str(input_path), str(output_path)]
            subprocess.run(cmd, check=True)
        else:
            raise e
