#!/usr/bin/env python3
"""Run a headless target and prove the real userspace bootstrap completes."""

import argparse
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.build.paths import get_manifest_dir, get_output_root
from tools.build.target_resolver import resolve_yaml_target
from tools.run.runner_qemu import build_qemu_command, load_run_manifest

DEFAULT_TARGET = REPO_ROOT / "delivery/targets/qemu/x86_64_desktop_headless.yaml"
FORBIDDEN_MARKERS = ("PANIC", "ASSERT", "FAULT", "Unhandled exception", "BOOT_FAIL:")

# Strict ordering required
EXPECTED_MARKERS = [
    "USER_INIT: ENTERED",
    "NAMESVC_MAIN_ENTER",
    "NAMESVC_READY",
    "PROCESS_MANAGER_LAUNCH",
    "PROCESS_MANAGER_READY",
    "BOOT_CLASS_CORE_READY",
]

class BootstrapSmokeError(RuntimeError):
    pass

def run_checked(command: list[str]) -> None:
    print(f"[bootstrap-smoke] command: {' '.join(command)}", flush=True)
    result = subprocess.run(command, cwd=REPO_ROOT, check=False)
    if result.returncode != 0:
        raise BootstrapSmokeError(
            f"command failed with status {result.returncode}: {' '.join(command)}"
        )

def run_smoke(args: argparse.Namespace) -> None:
    target_path = args.target.resolve()
    target = resolve_yaml_target(target_path)
    if target.arch != "x86_64" or target.run is None or not target.run.nographic:
        raise BootstrapSmokeError(
            "BOOT-REG-P0-002 requires an x86_64 headless QEMU target"
        )

    run_checked(
        [sys.executable, "tools/build.py", "build", "--target-yaml", str(target_path)]
    )
    run_checked(
        [sys.executable, "tools/build.py", "package", "--target-yaml", str(target_path)]
    )

    manifest_path = get_manifest_dir(target, REPO_ROOT) / "run-manifest.json"
    manifest = load_run_manifest(manifest_path)

    artifact_dir = (
        args.artifact_dir or get_output_root(target, REPO_ROOT) / "artifacts/bootstrap-smoke"
    )
    artifact_dir.mkdir(parents=True, exist_ok=True)
    serial_log = artifact_dir / "serial.log"
    qemu_log = artifact_dir / "qemu.log"
    for path in (serial_log, qemu_log):
        path.unlink(missing_ok=True)

    base = build_qemu_command(manifest, display_override="headless")
    command = []
    index = 0
    while index < len(base):
        argument = base[index]
        if argument == "-nographic":
            index += 1
            continue
        if argument in ("-serial", "-display"):
            index += 2
            continue
        command.append(argument)
        index += 1
    command.extend(
        [
            "-display",
            "none",
            "-serial",
            f"file:{serial_log}",
        ]
    )

    print(f"[bootstrap-smoke] target: {target_path}")
    print(f"[bootstrap-smoke] qemu: {' '.join(command)}", flush=True)
    deadline = time.monotonic() + args.timeout
    process: subprocess.Popen | None = None

    forced = False
    try:
        with qemu_log.open("wb") as diagnostics:
            process = subprocess.Popen(
                command,
                cwd=REPO_ROOT,
                stdin=subprocess.DEVNULL,
                stdout=diagnostics,
                stderr=subprocess.STDOUT,
            )

            marker_index = 0
            file_pos = 0

            while time.monotonic() < deadline:
                if serial_log.exists():
                    with serial_log.open("r", encoding="utf-8", errors="replace") as f:
                        f.seek(file_pos)
                        lines = f.readlines()
                        file_pos = f.tell()

                        for line in lines:
                            for forbidden in FORBIDDEN_MARKERS:
                                if forbidden in line:
                                    raise BootstrapSmokeError(
                                        f"forbidden serial marker observed: {forbidden}"
                                    )

                            if marker_index < len(EXPECTED_MARKERS):
                                if EXPECTED_MARKERS[marker_index] in line:
                                    marker_index += 1

                        if marker_index == len(EXPECTED_MARKERS):
                            print(f"[bootstrap-smoke] PASS: All markers observed in strict order. log={serial_log}")
                            return

                return_code = process.poll()
                if return_code is not None:
                    raise BootstrapSmokeError(
                        f"QEMU exited prematurely with status {return_code}"
                    )
                time.sleep(0.05)

            raise BootstrapSmokeError(
                f"timed out waiting for markers. Next expected: {EXPECTED_MARKERS[marker_index]}"
            )
    finally:
        if process is not None:
            if process.poll() is None:
                try:
                    process.terminate()
                    process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=2)
                    forced = True

def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run a headless target and prove the real userspace bootstrap completes."
    )
    parser.add_argument(
        "--target",
        type=Path,
        default=DEFAULT_TARGET,
        help=f"target YAML (default: {DEFAULT_TARGET.relative_to(REPO_ROOT)})",
    )
    parser.add_argument(
        "--artifact-dir",
        type=Path,
        help="artifact output directory (default: target build directory)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=60.0,
        help="bounded boot timeout in seconds (default: 60)",
    )
    return parser.parse_args(argv)

def main(argv: list[str] | None = None) -> int:
    try:
        run_smoke(parse_args(argv))
    except (BootstrapSmokeError, OSError, ValueError) as exc:
        print(f"[bootstrap-smoke] FAIL: {exc}", file=sys.stderr)
        return 1
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
