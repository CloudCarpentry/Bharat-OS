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


class FullScanRegressionTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)
        # Create minimal valid tree layout
        (self.root / "core/kernel").mkdir(parents=True)
        (self.root / "core/hal").mkdir(parents=True)
        (self.root / "core/lib").mkdir(parents=True)
        (self.root / "core/services").mkdir(parents=True)
        (self.root / "core/drivers").mkdir(parents=True)

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def run_scan(self) -> list[LINTER.Violation]:
        violations, _ = LINTER.scan(self.root)
        return violations

    def test_legal_kernel_to_hal_reference(self) -> None:
        source = self.root / "core/kernel/src/main.c"
        source.parent.mkdir(parents=True)
        source.write_text('#include "core/hal/hal.h"\n', encoding="utf-8")

        violations = self.run_scan()
        self.assertEqual(len(violations), 0)

    def test_illegal_library_to_service_reference(self) -> None:
        source = self.root / "core/lib/fs/fs.c"
        source.parent.mkdir(parents=True)
        source.write_text('#include "core/services/fs/fs_internal.h"\n', encoding="utf-8")

        violations = self.run_scan()
        self.assertTrue(any(v.rule == "layer-reference" and v.source_layer == "lib" and v.target_layer == "services" for v in violations))

    def test_illegal_service_to_driver_reference(self) -> None:
        source = self.root / "core/services/devmgr/main.c"
        source.parent.mkdir(parents=True)
        source.write_text('#include "core/drivers/dev.h"\n', encoding="utf-8")

        violations = self.run_scan()
        self.assertTrue(any(v.rule == "layer-reference" and v.source_layer == "services" and v.target_layer == "drivers" for v in violations))

    def test_service_like_names_inside_comments_and_strings_are_ignored(self) -> None:
        source = self.root / "core/lib/fs/fs.c"
        source.parent.mkdir(parents=True)
        source.write_text('// we call fsd_open_file here\nprintf("netmgr_started");\n/* multi\nline\ndevmgr_init\n*/\n', encoding="utf-8")

        violations = self.run_scan()
        self.assertEqual(len([v for v in violations if v.rule == "symbol-leakage"]), 0)

    def test_service_like_names_in_code_are_caught(self) -> None:
        source = self.root / "core/lib/fs/fs.c"
        source.parent.mkdir(parents=True)
        source.write_text('int fd = fsd_open_file(path);\n', encoding="utf-8")

        violations = self.run_scan()
        self.assertTrue(any(v.rule == "symbol-leakage" and v.include == "fsd_open_file" for v in violations))

    def test_freestanding_header_violations(self) -> None:
        source = self.root / "core/kernel/src/main.c"
        source.parent.mkdir(parents=True)
        source.write_text('#include <stdio.h>\n', encoding="utf-8")

        violations = self.run_scan()
        self.assertTrue(any(v.rule == "freestanding-header" and v.include == "stdio.h" for v in violations))

    def test_quoted_relative_includes_resolving_locally(self) -> None:
        source = self.root / "core/services/devmgr/main.c"
        target = self.root / "core/services/devmgr/local.h"
        source.parent.mkdir(parents=True)

        source.write_text('#include "local.h"\n', encoding="utf-8")
        target.write_text('// local header', encoding="utf-8")

        violations = self.run_scan()
        # Should resolve locally as 'services', which is allowed from 'services'
        self.assertEqual(len([v for v in violations if v.rule == "layer-reference"]), 0)


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
