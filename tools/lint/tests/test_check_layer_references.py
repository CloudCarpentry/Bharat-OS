#!/usr/bin/env python3
"""Regression tests for layer-reference include resolution."""

from __future__ import annotations

import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


LINTER_PATH = Path(__file__).resolve().parents[1] / "check_layer_references.py"
SPEC = importlib.util.spec_from_file_location("check_layer_references", LINTER_PATH)
assert SPEC is not None and SPEC.loader is not None
LINTER = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = LINTER
SPEC.loader.exec_module(LINTER)


class RelativeIncludeResolutionTests(unittest.TestCase):
    def test_kernel_relative_include_resolves_to_lib_layer(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source = root / "core/kernel/src/tests/example.c"
            target = root / "core/lib/msg/crc.h"
            source.parent.mkdir(parents=True)
            target.parent.mkdir(parents=True)
            source.write_text('#include "../../../lib/msg/crc.h"\n', encoding="utf-8")
            target.write_text("", encoding="utf-8")

            layer = LINTER.include_target_layer(
                root,
                source,
                "kernel",
                "../../../lib/msg/crc.h",
                True,
            )

            self.assertEqual(layer, "lib")

    def test_kernel_private_lib_include_remains_kernel_layer(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source = root / "core/kernel/src/example.c"
            target = root / "core/kernel/include/lib/base/string.h"
            source.parent.mkdir(parents=True)
            target.parent.mkdir(parents=True)
            target.write_text("", encoding="utf-8")

            layer = LINTER.include_target_layer(
                root, source, "kernel", "lib/base/string.h", True
            )

            self.assertEqual(layer, "kernel")


class CommandLineBaselineTests(unittest.TestCase):
    def test_checked_in_baseline_is_applied_by_default(self) -> None:
        result = subprocess.run(
            [sys.executable, str(LINTER_PATH), "--strict"],
            cwd=LINTER_PATH.parents[2],
            check=False,
            capture_output=True,
            text=True,
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("Violations found: 0", result.stdout)
        self.assertRegex(result.stdout, r"Known baseline debt: [1-9][0-9]*")

    def test_no_baseline_exposes_known_debt(self) -> None:
        result = subprocess.run(
            [sys.executable, str(LINTER_PATH), "--strict", "--no-baseline"],
            cwd=LINTER_PATH.parents[2],
            check=False,
            capture_output=True,
            text=True,
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertRegex(result.stdout, r"Violations found: [1-9][0-9]*")


if __name__ == "__main__":
    unittest.main()
