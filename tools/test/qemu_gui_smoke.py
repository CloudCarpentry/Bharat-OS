#!/usr/bin/env python3
"""Build a GUI target and prove that QEMU scanout contains a real frame."""

from __future__ import annotations

import argparse
import json
import re
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import BinaryIO

REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.build.paths import get_manifest_dir, get_output_root
from tools.build.target_resolver import resolve_yaml_target
from tools.run.runner_qemu import build_qemu_command, load_run_manifest

DEFAULT_TARGET = REPO_ROOT / "delivery/targets/qemu/x86_64_showcase_gui.yaml"
DISPLAY_MARKER = "[gui] display-ready"
FRAME_MARKER = "[gui] first-frame-presented"
MODE_PATTERN = re.compile(r"\[gui\] display-ready width=(\d+) height=(\d+)")
FORBIDDEN_MARKERS = ("PANIC", "ASSERT", "FAULT", "Unhandled exception")


class GuiSmokeError(RuntimeError):
    """A stable, user-facing visual smoke failure."""


@dataclass(frozen=True)
class PpmImage:
    width: int
    height: int
    pixels: bytes


def _ppm_token(stream: BinaryIO) -> bytes:
    token = bytearray()
    while True:
        byte = stream.read(1)
        if not byte:
            raise GuiSmokeError("malformed PPM: unexpected end of header")
        if byte == b"#":
            stream.readline()
            continue
        if not byte.isspace():
            token.extend(byte)
            break
    while True:
        byte = stream.read(1)
        if not byte or byte.isspace():
            return bytes(token)
        token.extend(byte)


def read_ppm(path: Path) -> PpmImage:
    """Read the binary P6 subset emitted by QEMU's screendump command."""
    try:
        with path.open("rb") as stream:
            magic = _ppm_token(stream)
            width_raw = _ppm_token(stream)
            height_raw = _ppm_token(stream)
            max_value_raw = _ppm_token(stream)
            if magic != b"P6":
                raise GuiSmokeError("malformed PPM: expected P6 magic")
            try:
                width = int(width_raw)
                height = int(height_raw)
                max_value = int(max_value_raw)
            except ValueError as exc:
                raise GuiSmokeError("malformed PPM: non-numeric dimensions") from exc
            if width <= 0 or height <= 0 or max_value != 255:
                raise GuiSmokeError("malformed PPM: invalid dimensions or max value")
            expected_size = width * height * 3
            pixels = stream.read()
    except OSError as exc:
        raise GuiSmokeError(f"cannot read screenshot: {exc}") from exc
    if len(pixels) != expected_size:
        raise GuiSmokeError(
            f"malformed PPM: expected {expected_size} pixel bytes, got {len(pixels)}"
        )
    return PpmImage(width, height, pixels)


def validate_frame(
    image: PpmImage,
    expected_width: int,
    expected_height: int,
    *,
    minimum_colors: int = 8,
    minimum_non_dominant_ratio: float = 0.01,
) -> tuple[int, float]:
    """Reject dimensions that disagree with the guest and blank/uniform scanout."""
    if (image.width, image.height) != (expected_width, expected_height):
        raise GuiSmokeError(
            "screenshot dimensions "
            f"{image.width}x{image.height} do not match guest mode "
            f"{expected_width}x{expected_height}"
        )
    counts: dict[bytes, int] = {}
    for offset in range(0, len(image.pixels), 3):
        pixel = image.pixels[offset : offset + 3]
        counts[pixel] = counts.get(pixel, 0) + 1
    pixel_count = image.width * image.height
    dominant = max(counts.values())
    non_dominant_ratio = (pixel_count - dominant) / pixel_count
    if len(counts) < minimum_colors or non_dominant_ratio < minimum_non_dominant_ratio:
        raise GuiSmokeError(
            "scanout is blank or uniform: "
            f"colors={len(counts)} non_dominant_ratio={non_dominant_ratio:.6f}; "
            f"required colors>={minimum_colors} ratio>={minimum_non_dominant_ratio:.6f}"
        )
    return len(counts), non_dominant_ratio


def make_qemu_command(manifest: dict, serial_log: Path, qmp_socket: Path) -> list[str]:
    """Construct a windowless QEMU command while retaining the emulated display."""
    base = build_qemu_command(manifest, display_override="headless")
    command: list[str] = []
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
            "-qmp",
            f"unix:{qmp_socket},server=on,wait=off",
        ]
    )
    return command


class QmpClient:
    def __init__(self, path: Path, deadline: float):
        self._socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        while True:
            try:
                self._socket.connect(str(path))
                break
            except (FileNotFoundError, ConnectionRefusedError):
                if time.monotonic() >= deadline:
                    self._socket.close()
                    raise GuiSmokeError("timed out connecting to QMP")
                time.sleep(0.05)
        remaining = max(0.1, deadline - time.monotonic())
        self._socket.settimeout(remaining)
        self._stream = self._socket.makefile("rwb", buffering=0)
        greeting = self._read_response()
        if "QMP" not in greeting:
            self.close()
            raise GuiSmokeError("QMP greeting was missing")
        self.execute("qmp_capabilities")

    def _read_response(self) -> dict:
        while True:
            line = self._stream.readline()
            if not line:
                raise GuiSmokeError("QMP connection closed unexpectedly")
            try:
                response = json.loads(line)
            except json.JSONDecodeError as exc:
                raise GuiSmokeError("QMP returned malformed JSON") from exc
            if "event" not in response:
                return response

    def execute(self, name: str, arguments: dict | None = None) -> dict:
        request: dict[str, object] = {"execute": name}
        if arguments:
            request["arguments"] = arguments
        self._stream.write(json.dumps(request).encode("utf-8") + b"\r\n")
        response = self._read_response()
        if "error" in response:
            description = response["error"].get("desc", "unknown QMP error")
            raise GuiSmokeError(f"QMP {name} failed: {description}")
        return response

    def close(self) -> None:
        try:
            self._stream.close()
        finally:
            self._socket.close()


def wait_for_markers(serial_log: Path, process: subprocess.Popen, deadline: float) -> tuple[int, int]:
    observed_display = False
    observed_frame = False
    width = 0
    height = 0
    consumed = 0
    while time.monotonic() < deadline:
        if serial_log.exists():
            content = serial_log.read_text(encoding="utf-8", errors="replace")
            new_content = content[consumed:]
            consumed = len(content)
            for forbidden in FORBIDDEN_MARKERS:
                if forbidden in new_content:
                    raise GuiSmokeError(f"forbidden serial marker observed: {forbidden}")
            match = MODE_PATTERN.search(content)
            if match:
                width, height = int(match.group(1)), int(match.group(2))
                observed_display = True
            observed_frame = FRAME_MARKER in content
            if observed_display and observed_frame:
                return width, height
        return_code = process.poll()
        if return_code is not None:
            raise GuiSmokeError(
                f"QEMU exited with status {return_code} before the first-frame markers"
            )
        time.sleep(0.05)
    missing = []
    if not observed_display:
        missing.append(DISPLAY_MARKER)
    if not observed_frame:
        missing.append(FRAME_MARKER)
    raise GuiSmokeError(f"timed out waiting for serial markers: {', '.join(missing)}")


def stop_qemu(process: subprocess.Popen, qmp: QmpClient | None) -> bool:
    """Request a clean QMP shutdown, falling back to bounded termination."""
    forced = False
    if process.poll() is not None:
        return forced
    if qmp is not None:
        try:
            qmp.execute("quit")
        except (GuiSmokeError, OSError):
            forced = True
    try:
        process.wait(timeout=3)
    except subprocess.TimeoutExpired:
        forced = True
        process.terminate()
        try:
            process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=2)
    return forced


def run_checked(command: list[str]) -> None:
    print(f"[gui-smoke] command: {' '.join(command)}", flush=True)
    result = subprocess.run(command, cwd=REPO_ROOT, check=False)
    if result.returncode != 0:
        raise GuiSmokeError(
            f"command failed with status {result.returncode}: {' '.join(command)}"
        )


def run_smoke(args: argparse.Namespace) -> None:
    target_path = args.target.resolve()
    target = resolve_yaml_target(target_path)
    if target.arch != "x86_64" or target.run is None or target.run.nographic:
        raise GuiSmokeError("GUI-002 currently requires an x86_64 graphical QEMU target")

    run_checked([sys.executable, "tools/build.py", "build", "--target-yaml", str(target_path)])
    run_checked([sys.executable, "tools/build.py", "package", "--target-yaml", str(target_path)])

    manifest_path = get_manifest_dir(target, REPO_ROOT) / "run-manifest.json"
    manifest = load_run_manifest(manifest_path)
    artifact_dir = args.artifact_dir or get_output_root(target, REPO_ROOT) / "artifacts/gui-smoke"
    artifact_dir.mkdir(parents=True, exist_ok=True)
    serial_log = artifact_dir / "serial.log"
    qemu_log = artifact_dir / "qemu.log"
    screenshot = artifact_dir / "first-frame.ppm"
    qmp_socket = artifact_dir / "qmp.sock"
    for path in (serial_log, qemu_log, screenshot, qmp_socket):
        path.unlink(missing_ok=True)

    command = make_qemu_command(manifest, serial_log, qmp_socket)
    print(f"[gui-smoke] target: {target_path}")
    print(f"[gui-smoke] qemu: {' '.join(command)}", flush=True)
    deadline = time.monotonic() + args.timeout
    process: subprocess.Popen | None = None
    qmp: QmpClient | None = None
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
            qmp = QmpClient(qmp_socket, deadline)
            width, height = wait_for_markers(serial_log, process, deadline)
            qmp.execute("screendump", {"filename": str(screenshot), "format": "ppm"})
            image = read_ppm(screenshot)
            color_count, ratio = validate_frame(
                image,
                width,
                height,
                minimum_colors=args.minimum_colors,
                minimum_non_dominant_ratio=args.minimum_non_dominant_ratio,
            )
            print(
                f"[gui-smoke] PASS target={target.name} mode={width}x{height} "
                f"colors={color_count} non_dominant_ratio={ratio:.6f} "
                f"screenshot={screenshot} serial={serial_log}"
            )
    finally:
        if process is not None:
            forced = stop_qemu(process, qmp)
        if qmp is not None:
            qmp.close()
        qmp_socket.unlink(missing_ok=True)
        if forced:
            print("[gui-smoke] warning: QEMU required forced termination", file=sys.stderr)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build a GUI target, capture QEMU scanout through QMP, and reject uniform frames."
    )
    parser.add_argument("--target", type=Path, default=DEFAULT_TARGET, help=f"target YAML (default: {DEFAULT_TARGET.relative_to(REPO_ROOT)})")
    parser.add_argument("--artifact-dir", type=Path, help="artifact output directory (default: target build directory)")
    parser.add_argument("--timeout", type=float, default=60.0, help="bounded boot/QMP timeout in seconds (default: 60)")
    parser.add_argument("--minimum-colors", type=int, default=8, help="minimum distinct RGB colors (default: 8)")
    parser.add_argument("--minimum-non-dominant-ratio", type=float, default=0.01, help="minimum pixels differing from the dominant color (default: 0.01)")
    args = parser.parse_args(argv)
    if args.timeout <= 0 or args.minimum_colors < 2 or not 0 < args.minimum_non_dominant_ratio <= 1:
        parser.error("timeout must be positive, colors >= 2, and ratio in (0, 1]")
    return args


def main(argv: list[str] | None = None) -> int:
    try:
        run_smoke(parse_args(argv))
    except (GuiSmokeError, OSError, ValueError) as exc:
        print(f"[gui-smoke] FAIL: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
