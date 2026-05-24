import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
import importlib.util
import sys

class TestCheckProfiles(unittest.TestCase):
    def setUp(self):
        # Dynamically find the repo root relative to this test file
        self.repo_root = Path(__file__).resolve().parents[3]
        self.script_path = self.repo_root / "tools/check_profiles.py"

    def _load_module(self):
        spec = importlib.util.spec_from_file_location("check_profiles", str(self.script_path))
        check_profiles = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(check_profiles)
        return check_profiles

    def test_checker_success(self):
        """Test that the checker returns success when all profiles are documented."""
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir_path = Path(tmpdir)

            # Create a mock build_config.json
            build_config = {
                "builds": {
                    "test_build": {"profile": "TEST_PROFILE"}
                }
            }
            config_file = tmpdir_path / "build_config.json"
            with open(config_file, "w") as f:
                json.dump(build_config, f)

            # Create a mock README.md
            readme_file = tmpdir_path / "README.md"
            with open(readme_file, "w") as f:
                f.write("# Bharat-OS\nThis project uses TEST_PROFILE.")

            check_profiles = self._load_module()

            # Mock the constants
            check_profiles.BUILD_CONFIG = config_file
            check_profiles.DOCS_DIRS = [readme_file]

            # The main function returns 0 on success
            # Redirect stdout to avoid cluttering test output
            original_stdout = sys.stdout
            sys.stdout = open(os.devnull, 'w')
            try:
                result = check_profiles.main()
            finally:
                sys.stdout.close()
                sys.stdout = original_stdout

            self.assertEqual(result, 0)

    def test_checker_failure(self):
        """Test that the checker returns failure when a profile is missing from docs."""
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir_path = Path(tmpdir)

            # Create a mock build_config.json
            build_config = {
                "builds": {
                    "test_build": {"profile": "MISSING_PROFILE"}
                }
            }
            config_file = tmpdir_path / "build_config.json"
            with open(config_file, "w") as f:
                json.dump(build_config, f)

            # Create a mock README.md without the profile
            readme_file = tmpdir_path / "README.md"
            with open(readme_file, "w") as f:
                f.write("# Bharat-OS\nNo profiles here.")

            check_profiles = self._load_module()

            # Mock the constants
            check_profiles.BUILD_CONFIG = config_file
            check_profiles.DOCS_DIRS = [readme_file]

            # The main function returns 1 on failure
            original_stdout = sys.stdout
            sys.stdout = open(os.devnull, 'w')
            try:
                result = check_profiles.main()
            finally:
                sys.stdout.close()
                sys.stdout = original_stdout

            self.assertEqual(result, 1)

    def test_checker_alias_success(self):
        """Test that the checker returns success when an alias is used in docs."""
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir_path = Path(tmpdir)

            # Create a mock build_config.json
            build_config = {
                "builds": {
                    "test_build": {"profile": "AUTOMOBILE"}
                }
            }
            config_file = tmpdir_path / "build_config.json"
            with open(config_file, "w") as f:
                json.dump(build_config, f)

            # Create a mock README.md using an alias
            readme_file = tmpdir_path / "README.md"
            with open(readme_file, "w") as f:
                f.write("# Bharat-OS\nThis project supports AUTOMOTIVE use cases.")

            check_profiles = self._load_module()

            # Mock the constants
            check_profiles.BUILD_CONFIG = config_file
            check_profiles.DOCS_DIRS = [readme_file]

            # AUTOMOTIVE is an alias for AUTOMOBILE
            original_stdout = sys.stdout
            sys.stdout = open(os.devnull, 'w')
            try:
                result = check_profiles.main()
            finally:
                sys.stdout.close()
                sys.stdout = original_stdout

            self.assertEqual(result, 0)

if __name__ == "__main__":
    unittest.main()
