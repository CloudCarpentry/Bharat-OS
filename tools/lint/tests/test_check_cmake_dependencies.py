#!/usr/bin/env python3
"""Regression tests for CMake dependency-linter baseline handling."""

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


LINTER_PATH = Path(__file__).resolve().parents[1] / "check_cmake_dependencies.py"
SPEC = importlib.util.spec_from_file_location("check_cmake_dependencies", LINTER_PATH)
assert SPEC is not None and SPEC.loader is not None
LINTER = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = LINTER
SPEC.loader.exec_module(LINTER)


class CommandLineBaselineTests(unittest.TestCase):
    def run_main(self, arguments: list[str]) -> int:
        with tempfile.TemporaryDirectory() as temp_dir:
            with (
                mock.patch.object(sys, "argv", [str(LINTER_PATH), *arguments]),
                mock.patch.object(LINTER, "register_query"),
                mock.patch.object(LINTER, "run_configure", return_value=True),
                mock.patch.object(
                    LINTER,
                    "analyze_dependencies",
                    side_effect=lambda _build_dir, baseline: [] if baseline else [
                        {
                            "source_target": "kernel.elf",
                            "source_layer": "kernel",
                            "target_target": "subsys_manager",
                            "target_layer": "services",
                            "message": "upward dependency",
                        }
                    ],
                ),
            ):
                try:
                    LINTER.main()
                except SystemExit as error:
                    return int(error.code)
        self.fail("main() did not exit")

    def test_checked_in_baseline_is_applied_by_default(self) -> None:
        self.assertEqual(self.run_main(["--strict"]), 0)

    def test_no_baseline_exposes_known_debt(self) -> None:
        self.assertEqual(self.run_main(["--strict", "--no-baseline"]), 1)


if __name__ == "__main__":
    unittest.main()
