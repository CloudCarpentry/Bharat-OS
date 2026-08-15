#!/usr/bin/env python3
"""Build, run, validate, and export deterministic HMEM QEMU evidence."""

import argparse
import csv
import json
import subprocess
import sys
import threading
import time
from queue import Empty, Queue
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))

from tools.run.runner_qemu import build_qemu_command, load_run_manifest

TARGETS = (
    "x86_64_hmem_bench_release.yaml",
    "arm64_hmem_bench_release.yaml",
    "riscv64_hmem_bench_release.yaml",
)
REQUIRED = (
    "TEST", "ITERATIONS", "BASELINE_COPIES", "HMEM_COPIES",
    "BASELINE_BYTES_COPIED", "HMEM_BYTES_COPIED", "BASELINE_CHECKSUM",
    "HMEM_CHECKSUM", "RESULT",
)


def parse_output(output: str) -> dict:
    start = output.find("BH_BENCH:START")
    complete = output.find("BH_BENCH:COMPLETE", start)
    if start < 0 or complete < 0:
        raise ValueError("benchmark START/COMPLETE markers not observed")
    result = {}
    for line in output[start:complete].splitlines():
        if line.startswith("BH_BENCH:") and "=" in line:
            key, value = line[len("BH_BENCH:"):].split("=", 1)
            if key in result:
                raise ValueError(f"duplicate benchmark key: {key}")
            result[key] = int(value) if value.isdigit() else value
    missing = sorted(set(REQUIRED) - result.keys())
    if missing:
        raise ValueError(f"missing benchmark keys: {', '.join(missing)}")
    if result["RESULT"] != "PASS":
        raise ValueError("guest benchmark reported failure")
    if result["BASELINE_CHECKSUM"] != result["HMEM_CHECKSUM"]:
        raise ValueError("baseline and HMEM checksums differ")
    if result["HMEM_COPIES"] != 0 or result["HMEM_BYTES_COPIED"] != 0:
        raise ValueError("HMEM handoff was not zero-copy")
    return result


def run_target(target: Path, timeout: int) -> dict:
    for action in ("build", "package"):
        build = [sys.executable, "tools/build.py", action, "--target-yaml", str(target)]
        print(f"BH_BENCH_RUN:COMMAND={' '.join(build)}", flush=True)
        subprocess.run(build, cwd=REPO, check=True)
    from tools.build.target_resolver import resolve_yaml_target
    from tools.build.paths import get_manifest_dir
    resolved = resolve_yaml_target(target)
    manifest = get_manifest_dir(resolved, REPO) / "run-manifest.json"
    command = build_qemu_command(load_run_manifest(manifest), "headless")
    print(f"BH_BENCH_RUN:COMMAND={' '.join(command)}", flush=True)
    process = subprocess.Popen(command, cwd=REPO, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True)
    lines = []
    deadline = time.monotonic() + timeout
    output_queue = Queue()

    def read_output() -> None:
        for output_line in process.stdout:
            output_queue.put(output_line)

    reader = threading.Thread(target=read_output, daemon=True)
    reader.start()
    try:
        while time.monotonic() < deadline:
            try:
                line = output_queue.get(timeout=min(0.25, deadline - time.monotonic()))
            except Empty:
                if process.poll() is not None:
                    break
                continue
            if line:
                print(line, end="")
                lines.append(line)
                if "BH_BENCH:COMPLETE" in line:
                    break
        else:
            raise TimeoutError(f"benchmark timed out after {timeout}s")
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
    parsed = parse_output("".join(lines))
    return {"arch": resolved.arch, "target": resolved.name,
            "benchmark": parsed.pop("TEST"), "metrics": parsed,
            "timing_scope": "QEMU software-overhead indicator"}


def write_results(results: list[dict], output: Path) -> None:
    output.mkdir(parents=True, exist_ok=True)
    (output / "hmem_bench.json").write_text(json.dumps(results, indent=2) + "\n")
    keys = sorted({key for result in results for key in result["metrics"]})
    with (output / "hmem_bench.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=["arch", "target", "benchmark"] + keys)
        writer.writeheader()
        for result in results:
            writer.writerow({"arch": result["arch"], "target": result["target"],
                             "benchmark": result["benchmark"], **result["metrics"]})


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument("--target", type=Path, help="benchmark target YAML")
    selection.add_argument("--matrix", action="store_true",
                           help="run x86_64, arm64, and riscv64 benchmark targets")
    parser.add_argument("--output", type=Path, default=Path("build/bench-results"))
    parser.add_argument("--timeout", type=int, default=120)
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    targets = ([REPO / "delivery/targets/qemu" / name for name in TARGETS]
               if args.matrix else [args.target.resolve()])
    missing = [str(target) for target in targets if not target.is_file()]
    if missing:
        parser.error("missing required target(s): " + ", ".join(missing))
    try:
        results = [run_target(target, args.timeout) for target in targets]
        write_results(results, (REPO / args.output).resolve())
    except (OSError, ValueError, TimeoutError, subprocess.CalledProcessError) as error:
        print(f"BH_BENCH_RUN:RESULT=FAIL REASON={error}", file=sys.stderr)
        return 1
    print("BH_BENCH_RUN:RESULT=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
