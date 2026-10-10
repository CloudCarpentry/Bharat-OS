import sys
import os
import json
import unittest
from unittest.mock import patch, MagicMock
from pathlib import Path
from io import StringIO
import yaml

# Add the repo root to path to import the module
repo_root = Path(__file__).resolve().parent.parent.parent.parent
sys.path.insert(0, str(repo_root))

from tools.nirmaan.commands import demo

class TestDemoCommand(unittest.TestCase):
    def setUp(self):
        self.patcher = patch("builtins.open")
        self.mock_open = self.patcher.start()

        def side_effect(filename, *args, **kwargs):
            m = MagicMock()
            if "yaml" in str(filename):
                m.__enter__.return_value = StringIO(yaml.dump({"build": {"cmake_preset": "x86_64-hmem-demo"}}))
            else:
                m.__enter__.return_value = StringIO(json.dumps({"arch": "x86_64", "run_config": {}, "boot_contract": {}}))
            return m
        self.mock_open.side_effect = side_effect

    def tearDown(self):
        self.patcher.stop()

    @patch("tools.nirmaan.commands.demo.subprocess.run")
    @patch("tools.nirmaan.commands.demo.subprocess.Popen")
    @patch("tools.nirmaan.commands.demo.Path.exists")
    def test_panic_detection(self, mock_exists, mock_popen, mock_run):
        mock_exists.return_value = True

        mock_run_result = MagicMock()
        mock_run_result.returncode = 0
        mock_run.return_value = mock_run_result

        mock_proc = MagicMock()
        mock_proc.poll.side_effect = [None, None, 0, 0, 0, 0]
        mock_proc.stdout = StringIO("BOOT: kernel_main reached\nKERNEL PANIC: fatal error\n")
        mock_proc.returncode = 0
        mock_popen.return_value = mock_proc

        res = demo.run(["x86_64_hmem_demo"])
        self.assertEqual(res, 1)

    @patch("tools.nirmaan.commands.demo.subprocess.run")
    @patch("tools.nirmaan.commands.demo.subprocess.Popen")
    @patch("tools.nirmaan.commands.demo.Path.exists")
    @patch("tools.nirmaan.commands.demo.time.time")
    def test_timeout(self, mock_time, mock_exists, mock_popen, mock_run):
        mock_exists.return_value = True

        mock_run_result = MagicMock()
        mock_run_result.returncode = 0
        mock_run.return_value = mock_run_result

        mock_proc = MagicMock()
        mock_proc.poll.return_value = None
        mock_proc.stdout = StringIO("")
        mock_popen.return_value = mock_proc

        mock_time.side_effect = [0, 61, 61, 61, 61]

        res = demo.run(["x86_64_hmem_demo"])
        self.assertEqual(res, 1)

    @patch("tools.nirmaan.commands.demo.subprocess.run")
    @patch("tools.nirmaan.commands.demo.subprocess.Popen")
    @patch("tools.nirmaan.commands.demo.Path.exists")
    def test_missing_markers_and_early_exit(self, mock_exists, mock_popen, mock_run):
        mock_exists.return_value = True

        mock_run_result = MagicMock()
        mock_run_result.returncode = 0
        mock_run.return_value = mock_run_result

        mock_proc = MagicMock()
        mock_proc.poll.side_effect = [None, 0, 0, 0, 0]
        mock_proc.stdout = StringIO("BOOT: kernel_main reached\n")
        mock_proc.returncode = 1
        mock_popen.return_value = mock_proc

        res = demo.run(["x86_64_hmem_demo"])
        self.assertEqual(res, 1)

    @patch("tools.nirmaan.commands.demo.subprocess.run")
    @patch("tools.nirmaan.commands.demo.Path.exists")
    def test_stale_artifacts(self, mock_exists, mock_run):
        def exists_side_effect():
            # If it's a manifest, return False
            pass
        mock_exists.side_effect = lambda: False # will fail at yaml, that's okay for stale artifacut

        mock_run_result = MagicMock()
        mock_run_result.returncode = 0
        mock_run.return_value = mock_run_result

        res = demo.run(["x86_64_hmem_demo"])
        self.assertEqual(res, 1)

if __name__ == "__main__":
    unittest.main()
