import os
import sys
import subprocess
import tempfile
import shutil
import unittest

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LINTER_SCRIPT = os.path.join(SCRIPT_DIR, "..", "check_placement.py")

class TestCheckPlacement(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()

        # Create minimal directory structure mocking the repo
        self.kernel_dir = os.path.join(self.temp_dir, "kernel")
        self.services_dir = os.path.join(self.temp_dir, "services")
        self.tools_dir = os.path.join(self.temp_dir, "tools")
        self.my_tools_dir = os.path.join(self.temp_dir, "my_tools_component")

        os.makedirs(self.kernel_dir)
        os.makedirs(self.services_dir)
        os.makedirs(self.tools_dir)
        os.makedirs(self.my_tools_dir)

        # Override REPO_ROOT for the test using an environment variable trick,
        # but since check_placement.py calculates REPO_ROOT from __file__,
        # we will instead copy the script into the temp_dir structure
        # wait, that's complex. Let's just create a modified copy or patch the file.

        # Better approach: We'll modify a copy of check_placement.py to point REPO_ROOT to our tempdir
        self.test_linter = os.path.join(self.temp_dir, "test_check_placement.py")
        with open(LINTER_SCRIPT, "r") as f:
            content = f.read()

        # Replace the REPO_ROOT definition
        content = content.replace(
            'REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))',
            f'REPO_ROOT = "{self.temp_dir}"'
        )

        with open(self.test_linter, "w") as f:
            f.write(content)
        os.chmod(self.test_linter, 0o755)

    def tearDown(self):
        shutil.rmtree(self.temp_dir)

    def run_linter(self):
        result = subprocess.run([sys.executable, self.test_linter], capture_output=True, text=True)
        return result.returncode, result.stdout

    def test_clean_repo(self):
        # A completely clean repo should pass
        with open(os.path.join(self.kernel_dir, "clean.c"), "w") as f:
            f.write("int main() { return 0; }\n")

        returncode, stdout = self.run_linter()
        self.assertEqual(returncode, 0, f"Expected clean run, got:\n{stdout}")

    def test_kernel_emulator_filename(self):
        # Should flag qemu in kernel filename
        with open(os.path.join(self.kernel_dir, "qemu_driver.c"), "w") as f:
            f.write("int x = 0;\n")

        returncode, stdout = self.run_linter()
        self.assertEqual(returncode, 1)
        self.assertIn("Emulator logic inside kernel filename", stdout)

    def test_kernel_emulator_content(self):
        # Should flag renode in kernel file content
        with open(os.path.join(self.kernel_dir, "test.c"), "w") as f:
            f.write("// some renode stuff\n")

        returncode, stdout = self.run_linter()
        self.assertEqual(returncode, 1)
        self.assertIn("Emulator logic inside kernel source", stdout)

    def test_services_hardware_driver(self):
        # Should flag hw_control in services
        with open(os.path.join(self.services_dir, "bad_service.c"), "w") as f:
            f.write("void hw_control_init() {}\n")

        returncode, stdout = self.run_linter()
        self.assertEqual(returncode, 1)
        self.assertIn("Suspected hardware driver logic in service", stdout)

    def test_memops_in_my_tools(self):
        # Should flag internal_memset in my_tools_component (since it's not strictly 'tools')
        with open(os.path.join(self.my_tools_dir, "test.c"), "w") as f:
            f.write("void test() { internal_memset(0, 0, 0); }\n")

        returncode, stdout = self.run_linter()
        self.assertEqual(returncode, 1)
        self.assertIn("Use of forbidden internal_memset/memcpy/memmove", stdout)

    def test_memops_in_tools_excluded(self):
        # Should NOT flag internal_memset in tools/
        with open(os.path.join(self.tools_dir, "test.c"), "w") as f:
            f.write("void test() { internal_memset(0, 0, 0); }\n")

        returncode, stdout = self.run_linter()
        self.assertEqual(returncode, 0)
        self.assertNotIn("Use of forbidden internal_memset/memcpy/memmove", stdout)

    def test_memops_not_on_S_files(self):
        # internal_memset should not be flagged on .S files
        with open(os.path.join(self.kernel_dir, "test.S"), "w") as f:
            f.write("internal_memset\n")

        returncode, stdout = self.run_linter()
        self.assertEqual(returncode, 0)

if __name__ == "__main__":
    unittest.main()
