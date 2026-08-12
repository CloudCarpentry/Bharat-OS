import importlib.util
from pathlib import Path

MODULE = Path(__file__).parents[3] / "tools/bench/run_hmem_bench.py"
SPEC = importlib.util.spec_from_file_location("hmem_bench_runner", MODULE)
runner = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(runner)


def test_parse_valid_zero_copy_result():
    data = """noise
BH_BENCH:START
BH_BENCH:TEST=PIPELINE_64K
BH_BENCH:ITERATIONS=10
BH_BENCH:BASELINE_COPIES=30
BH_BENCH:HMEM_COPIES=0
BH_BENCH:BASELINE_BYTES_COPIED=1966080
BH_BENCH:HMEM_BYTES_COPIED=0
BH_BENCH:BASELINE_CHECKSUM=42
BH_BENCH:HMEM_CHECKSUM=42
BH_BENCH:RESULT=PASS
BH_BENCH:COMPLETE
"""
    assert runner.parse_output(data)["BASELINE_COPIES"] == 30


def test_parse_rejects_checksum_mismatch():
    data = """BH_BENCH:START
BH_BENCH:TEST=X
BH_BENCH:ITERATIONS=1
BH_BENCH:BASELINE_COPIES=1
BH_BENCH:HMEM_COPIES=0
BH_BENCH:BASELINE_BYTES_COPIED=1
BH_BENCH:HMEM_BYTES_COPIED=0
BH_BENCH:BASELINE_CHECKSUM=1
BH_BENCH:HMEM_CHECKSUM=2
BH_BENCH:RESULT=PASS
BH_BENCH:COMPLETE
"""
    try:
        runner.parse_output(data)
    except ValueError as error:
        assert "checksums differ" in str(error)
    else:
        raise AssertionError("checksum mismatch accepted")
