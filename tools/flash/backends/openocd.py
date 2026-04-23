import subprocess


def execute_openocd(manifest: dict, dry_run: bool = False) -> int:
    flash_config = manifest.get("flash_config", {})
    probe = flash_config.get("probe", "unknown")
    artifacts = manifest.get("artifacts", {})
    flash_artifact = artifacts.get("flash_artifact")

    cmd = ["openocd"]
    if probe != "unknown":
        cmd.extend(["-f", f"interface/{probe}.cfg"])

    cmd.extend(["-c", f"program {flash_artifact} verify reset exit"])

    print(f"[OpenOCD] Prepared command: {' '.join(cmd)}")

    if dry_run:
        print("[OpenOCD] Dry-run enabled. Skipping actual execution.")
        return 0

    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error executing OpenOCD: {e}")
        return e.returncode

    return 0
