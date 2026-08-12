import json
from pathlib import Path
import subprocess
import tempfile
import unittest


class ImplementationMaturityGateTests(unittest.TestCase):
    checker = Path("tools/check_implementation_maturity.py")

    def run_case(self, maturity, profile, *allow):
        manifest = {
            "schema_version": 1,
            "implementations": [{
                "id": "test.component",
                "maturity": maturity,
                "sources": ["AGENTS.md"],
                "evidence": "test fixture",
            }],
        }
        with tempfile.NamedTemporaryFile("w", suffix=".json") as stream:
            json.dump(manifest, stream)
            stream.flush()
            command = ["python3", str(self.checker), "--manifest", stream.name,
                       "--profile", profile]
            for maturity_name in allow:
                command.extend(["--allow", maturity_name])
            return subprocess.run(command, text=True, capture_output=True, check=False)

    def test_production_real_passes(self):
        self.assertEqual(self.run_case("REAL", "RELEASE").returncode, 0)

    def test_production_stub_fails(self):
        result = self.run_case("STUB", "RELEASE")
        self.assertEqual(result.returncode, 1)
        self.assertIn("STUB is forbidden", result.stdout)

    def test_production_test_only_fails(self):
        result = self.run_case("TEST_ONLY", "HARDENED")
        self.assertEqual(result.returncode, 1)
        self.assertIn("TEST_ONLY is forbidden", result.stdout)

    def test_development_explicitly_allowing_stub_passes(self):
        self.assertEqual(self.run_case("STUB", "DEVELOPMENT", "STUB").returncode, 0)

    def test_development_without_allowance_fails_closed(self):
        self.assertEqual(self.run_case("STUB", "DEBUG").returncode, 1)


if __name__ == "__main__":
    unittest.main()
