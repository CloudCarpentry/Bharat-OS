import sys
import unittest
import re
from pathlib import Path
import yaml

# Adjust path to import check_boot_log
repo_root = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(repo_root))
sys.path.insert(0, str(repo_root / "tools" / "testing"))

from check_boot_log import BootLogParser

class TestBootLogParser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        contract_path = repo_root / "quality" / "contracts" / "boot" / "headless_boot_contract.yaml"
        with open(contract_path, "r") as f:
            cls.contract = yaml.safe_load(f)

    def test_nine_suspicious_patterns_strict(self):
        """Test that all 9 STRICT_SUSPICIOUS_PATTERNS are caught in strict mode."""
        # We'll use a target with empty forbidden list to see them clearly in suspicious_found,
        # or we just test them directly. Let's create a custom dummy contract to isolate behavior.
        dummy_contract = {
            "targets": {
                "test_target": {
                    "required": ["BOOT: success"],
                    "forbidden": [], # No forbidden markers so they don't get caught there
                    "allowed_skip": []
                }
            }
        }
        parser = BootLogParser(dummy_contract, "test_target", strict=True)

        test_cases = [
            ("This is a PANIC error", "PANIC"),
            ("An ASSERT occurred", "ASSERT"),
            ("A FAULT was raised", "FAULT"),
            ("Triggered General Protection Fault here", "General Protection Fault"),
            ("Got page fault on address 0x0", "page fault"),
            ("Double fault led to triple fault inside kernel", "triple fault"),
            ("Unhandled exception: division by zero", "Unhandled exception"),
            ("A kernel oops occurred!", "kernel oops"),
            ("Process terminated with segmentation fault", "segmentation fault"),
        ]

        for line, description in test_cases:
            with self.subTest(pattern=description):
                results = parser.parse([
                    "BOOT: success",
                    line
                ])
                self.assertIn(line, results["suspicious_found"], f"Failed to detect suspicious line: {line}")

    def test_clean_lines(self):
        """Test that normal, clean lines are not flagged as suspicious."""
        dummy_contract = {
            "targets": {
                "test_target": {
                    "required": ["BOOT: success"],
                    "forbidden": [],
                    "allowed_skip": []
                }
            }
        }
        parser = BootLogParser(dummy_contract, "test_target", strict=True)
        results = parser.parse([
            "BOOT: success",
            "This is a perfectly normal log line",
            "Initializing driver: serial_ns16550",
            "Memory region mapped successfully",
        ])
        self.assertEqual(len(results["suspicious_found"]), 0)

    def test_allowed_skips_exemption(self):
        """Test that allowed skips are exempt from strict suspicious checks."""
        dummy_contract = {
            "targets": {
                "test_target": {
                    "required": ["BOOT: success"],
                    "forbidden": [],
                    "allowed_skip": [
                        "SKIP: single core"
                    ]
                }
            }
        }
        parser = BootLogParser(dummy_contract, "test_target", strict=True)
        # Even if the line contains a suspicious term like "FAULT",
        # if it is an allowed skip, it should be skipped.
        results = parser.parse([
            "BOOT: success",
            "SKIP: single core - avoiding FAULT check"
        ])
        self.assertEqual(len(results["suspicious_found"]), 0)
        self.assertEqual(len(results["skips_found"]), 1)

    def test_forbidden_markers_suppression(self):
        """Test that lines already caught as forbidden are not duplicated in suspicious_found."""
        dummy_contract = {
            "targets": {
                "test_target": {
                    "required": ["BOOT: success"],
                    "forbidden": ["PANIC"],
                    "allowed_skip": []
                }
            }
        }
        parser = BootLogParser(dummy_contract, "test_target", strict=True)
        results = parser.parse([
            "BOOT: success",
            "A critical PANIC error occurred!"
        ])
        # Should be in forbidden_found, not suspicious_found
        self.assertEqual(len(results["forbidden_found"]), 1)
        self.assertEqual(len(results["suspicious_found"]), 0)

    def test_multiple_suspicious_patterns(self):
        """Test that lines matching multiple suspicious patterns are caught correctly."""
        dummy_contract = {
            "targets": {
                "test_target": {
                    "required": ["BOOT: success"],
                    "forbidden": [],
                    "allowed_skip": []
                }
            }
        }
        parser = BootLogParser(dummy_contract, "test_target", strict=True)
        line = "Both page fault and triple fault occurred!"
        results = parser.parse([
            "BOOT: success",
            line
        ])
        self.assertIn(line, results["suspicious_found"])
        self.assertEqual(len(results["suspicious_found"]), 2) # It matches both 'page fault' and 'triple fault', so it's added twice

    def test_equivalence(self):
        """Verify that parsed results are identical using a variety of inputs."""
        # This will verify that any optimizations we make preserve 100% exact outcome.
        parser = BootLogParser(self.contract, "x86_64_desktop_headless", strict=True)
        sample_log = [
            "BOOT: kernel_main reached",
            "BOOT: pmm initialized",
            "BOOT: vmm initialized",
            "[BOOT] Memory subsystem initialization complete",
            "[BOOT] Platform services initialization complete",
            "[BOOT] Runtime initialization complete",
            "[BOOT] Spawning first system service (sysmgr)...",
            "SKIP: sched_remote_ipi requires >=2 online cores",
            "page fault in user space",
            "Some clean line",
            "triple fault detected",
        ]
        results = parser.parse(sample_log)

        # Verify basic expected counts
        self.assertTrue(all(results["required"].values()))
        self.assertEqual(len(results["skips_found"]), 1)
        self.assertEqual(len(results["forbidden_found"]), 1) # triple fault is forbidden in contract
        # In strict mode: triple fault is forbidden, so it's not in suspicious.
        # page fault is not forbidden, so it is in suspicious.
        self.assertEqual(results["suspicious_found"], ["page fault in user space"])

if __name__ == "__main__":
    unittest.main()
