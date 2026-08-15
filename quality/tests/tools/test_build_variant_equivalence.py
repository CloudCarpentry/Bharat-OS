import json
import subprocess
import sys
from pathlib import Path

TOOL = Path(__file__).parents[3] / "tools" / "check_build_variant_equivalence.py"


def config(board="qemu-virt", extra_instrumentation=None):
    instrumentation = {
        "variant": "DEBUG", "assertions": "ON", "symbols": "ON",
        "optimization": "UNOPTIMIZED", "tracing": "OFF", "test_hooks": "OFF",
        "poisoning": "OFF", "invariant_checking": "ON",
    }
    instrumentation.update(extra_instrumentation or {})
    return {"schema_version": 1, "functional": {"board": board}, "instrumentation": instrumentation}


def run(tmp_path, left, right):
    paths = [tmp_path / "debug.json", tmp_path / "release.json"]
    for path, value in zip(paths, (left, right)):
        path.write_text(json.dumps(value), encoding="utf-8")
    return subprocess.run([sys.executable, str(TOOL), *map(str, paths)], capture_output=True, text=True)


def test_allows_only_whitelisted_instrumentation_differences(tmp_path):
    release = config(extra_instrumentation={"variant": "RELEASE", "assertions": "OFF", "symbols": "OFF", "optimization": "OPTIMIZED", "invariant_checking": "OFF"})
    assert run(tmp_path, config(), release).returncode == 0


def test_rejects_functional_difference(tmp_path):
    result = run(tmp_path, config(), config(board="different-board"))
    assert result.returncode == 1
    assert "board" in result.stdout


def test_rejects_unlisted_instrumentation_key(tmp_path):
    result = run(tmp_path, config(extra_instrumentation={"production_service": "OFF"}), config())
    assert result.returncode != 0
    assert "non-whitelisted" in result.stderr
