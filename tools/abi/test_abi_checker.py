import unittest
import os
import shutil
import json
import subprocess
import re
import sys

class TestSyscallABI(unittest.TestCase):
    def setUp(self):
        self.test_dir = os.path.abspath("temp_abi_test")
        os.makedirs(self.test_dir, exist_ok=True)
        self.syscall_def = os.path.join(self.test_dir, "syscall_table.def")
        self.manifest = os.path.join(self.test_dir, "syscalls.json")
        self.script = os.path.abspath("tools/abi/check_syscalls.py")
        self.common = os.path.abspath("tools/abi/common.py")

        shutil.copy(self.script, os.path.join(self.test_dir, "check_syscalls.py"))
        shutil.copy(self.common, os.path.join(self.test_dir, "common.py"))

    def tearDown(self):
        if os.path.exists(self.test_dir):
            shutil.rmtree(self.test_dir)

    def write_def(self, entries):
        with open(self.syscall_def, "w") as f:
            for name, num in entries:
                f.write(f"SYSCALL_DEF({name}, {num})\n")

    def patch_script(self):
        script_path = os.path.join(self.test_dir, "check_syscalls.py")
        with open(script_path, "r") as f:
            content = f.read()
        content = re.sub(r'SYSCALL_TABLE_PATH = ".*"', 'SYSCALL_TABLE_PATH = "./syscall_table.def"', content)
        content = re.sub(r'MANIFEST_PATH = ".*"', 'MANIFEST_PATH = "./syscalls.json"', content)
        with open(script_path, "w") as f:
            f.write(content)

    def run_command(self, cmd_flag):
        self.patch_script()
        # We need to add the test_dir to PYTHONPATH so check_syscalls.py can find common.py
        env = os.environ.copy()
        env["PYTHONPATH"] = self.test_dir
        result = subprocess.run([sys.executable, "check_syscalls.py", cmd_flag],
                                capture_output=True, text=True, cwd=self.test_dir, env=env)
        return result

    def test_append_only_passes(self):
        self.write_def([("A", 1), ("B", 2)])
        self.run_command("--update")

        self.write_def([("A", 1), ("B", 2), ("C", 3)])
        res = self.run_command("--check")
        self.assertEqual(res.returncode, 0, f"STDOUT: {res.stdout}\nSTDERR: {res.stderr}")

    def test_renumber_fails(self):
        self.write_def([("A", 1), ("B", 2)])
        self.run_command("--update")

        self.write_def([("A", 1), ("B", 3)])
        res = self.run_command("--check")
        self.assertNotEqual(res.returncode, 0)
        self.assertIn("was removed", res.stderr)

    def test_delete_fails(self):
        self.write_def([("A", 1), ("B", 2)])
        self.run_command("--update")

        self.write_def([("A", 1)])
        res = self.run_command("--check")
        self.assertNotEqual(res.returncode, 0)
        self.assertIn("was removed", res.stderr)

    def test_duplicate_number_fails(self):
        self.write_def([("A", 1), ("B", 1)])
        res = self.run_command("--update") # Or any command that triggers manifest generation
        self.assertNotEqual(res.returncode, 0)
        self.assertIn("Duplicate syscall number", res.stderr)

    def test_duplicate_name_fails(self):
        self.write_def([("A", 1), ("A", 2)])
        res = self.run_command("--update")
        self.assertNotEqual(res.returncode, 0)
        self.assertIn("Duplicate syscall name", res.stderr)

if __name__ == "__main__":
    unittest.main()
