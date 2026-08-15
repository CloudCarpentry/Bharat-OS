#!/usr/bin/env python3
import importlib.util
import tempfile
import unittest
from pathlib import Path

PATH = Path(__file__).resolve().parents[1] / "check_hal_dependency_direction.py"
SPEC = importlib.util.spec_from_file_location("hal_direction", PATH)
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)

class HalDirectionTests(unittest.TestCase):
    def test_rejects_forbidden_imports(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            hal = root / "core/hal/common/a.c"
            lib = root / "core/lib/a.c"
            hal.parent.mkdir(parents=True)
            lib.parent.mkdir(parents=True)
            hal.write_text('#include "arch/private.h"\n', encoding="utf-8")
            lib.write_text('#include "hal/hal_internal.h"\n', encoding="utf-8")
            self.assertEqual(len(MODULE.violations(root)), 2)

    def test_allows_public_contracts(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            hal = root / "core/hal/common/a.c"
            lib = root / "core/lib/a.c"
            hal.parent.mkdir(parents=True)
            lib.parent.mkdir(parents=True)
            hal.write_text('#include "hal/hal_cpu_features.h"\n', encoding="utf-8")
            lib.write_text('#include "bharat/uapi/types.h"\n', encoding="utf-8")
            self.assertEqual(MODULE.violations(root), [])

if __name__ == "__main__":
    unittest.main()
