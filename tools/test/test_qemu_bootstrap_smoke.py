import sys
import tempfile
from pathlib import Path
REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

import unittest
from unittest import mock
import time
from tools.test.qemu_bootstrap_smoke import EXPECTED_MARKERS, FORBIDDEN_MARKERS, BootstrapSmokeError, run_smoke
import argparse

class BootstrapSmokeTests(unittest.TestCase):
    def test_expected_markers(self):
        self.assertIn("USER_INIT: ENTERED", EXPECTED_MARKERS)
        self.assertIn("NAMESVC_MAIN_ENTER", EXPECTED_MARKERS)
        self.assertIn("NAMESVC_READY", EXPECTED_MARKERS)
        self.assertIn("PROCESS_MANAGER_LAUNCH", EXPECTED_MARKERS)
        self.assertIn("PROCESS_MANAGER_READY", EXPECTED_MARKERS)
        self.assertIn("BOOT_CLASS_CORE_READY", EXPECTED_MARKERS)

    @mock.patch("tools.test.qemu_bootstrap_smoke.run_checked")
    @mock.patch("tools.test.qemu_bootstrap_smoke.subprocess.Popen")
    @mock.patch("tools.test.qemu_bootstrap_smoke.load_run_manifest")
    @mock.patch("tools.test.qemu_bootstrap_smoke.get_manifest_dir")
    @mock.patch("tools.test.qemu_bootstrap_smoke.get_output_root")
    @mock.patch("tools.test.qemu_bootstrap_smoke.resolve_yaml_target")
    def test_mocked_success(self, mock_resolve_target, mock_get_output_root, mock_get_manifest_dir, mock_load_manifest, mock_popen, mock_run_checked):
        mock_target = mock.Mock()
        mock_target.arch = "x86_64"
        mock_target.run.nographic = True
        mock_resolve_target.return_value = mock_target
        mock_load_manifest.return_value = {"arch": "x86_64", "run_config": {}, "artifacts": {}}

        with tempfile.TemporaryDirectory() as d:
            mock_get_manifest_dir.return_value = Path(d)
            mock_get_output_root.return_value = Path(d)

            mock_process = mock.Mock()
            mock_process.poll.return_value = None
            mock_popen.return_value = mock_process

            serial_log = Path(d) / "artifacts/bootstrap-smoke/serial.log"
            serial_log.parent.mkdir(parents=True, exist_ok=True)

            def mock_poll():
                serial_log.write_text("\n".join(EXPECTED_MARKERS))
                return None
            mock_process.poll.side_effect = mock_poll

            args = argparse.Namespace(target=Path("dummy.yaml"), artifact_dir=Path(d)/"artifacts/bootstrap-smoke", timeout=5.0)

            run_smoke(args)

    @mock.patch("tools.test.qemu_bootstrap_smoke.run_checked")
    @mock.patch("tools.test.qemu_bootstrap_smoke.subprocess.Popen")
    @mock.patch("tools.test.qemu_bootstrap_smoke.load_run_manifest")
    @mock.patch("tools.test.qemu_bootstrap_smoke.get_manifest_dir")
    @mock.patch("tools.test.qemu_bootstrap_smoke.get_output_root")
    @mock.patch("tools.test.qemu_bootstrap_smoke.resolve_yaml_target")
    def test_mocked_forbidden(self, mock_resolve_target, mock_get_output_root, mock_get_manifest_dir, mock_load_manifest, mock_popen, mock_run_checked):
        mock_target = mock.Mock()
        mock_target.arch = "x86_64"
        mock_target.run.nographic = True
        mock_resolve_target.return_value = mock_target
        mock_load_manifest.return_value = {"arch": "x86_64", "run_config": {}, "artifacts": {}}

        with tempfile.TemporaryDirectory() as d:
            mock_get_manifest_dir.return_value = Path(d)
            mock_get_output_root.return_value = Path(d)

            mock_process = mock.Mock()
            mock_popen.return_value = mock_process

            serial_log = Path(d) / "artifacts/bootstrap-smoke/serial.log"
            serial_log.parent.mkdir(parents=True, exist_ok=True)

            def mock_poll():
                serial_log.write_text("USER_INIT: ENTERED\nPANIC: kernel panic\n")
                return None
            mock_process.poll.side_effect = mock_poll

            args = argparse.Namespace(target=Path("dummy.yaml"), artifact_dir=Path(d)/"artifacts/bootstrap-smoke", timeout=5.0)

            with self.assertRaisesRegex(BootstrapSmokeError, "forbidden"):
                run_smoke(args)

    @mock.patch("tools.test.qemu_bootstrap_smoke.run_checked")
    @mock.patch("tools.test.qemu_bootstrap_smoke.subprocess.Popen")
    @mock.patch("tools.test.qemu_bootstrap_smoke.load_run_manifest")
    @mock.patch("tools.test.qemu_bootstrap_smoke.get_manifest_dir")
    @mock.patch("tools.test.qemu_bootstrap_smoke.get_output_root")
    @mock.patch("tools.test.qemu_bootstrap_smoke.resolve_yaml_target")
    def test_mocked_out_of_order(self, mock_resolve_target, mock_get_output_root, mock_get_manifest_dir, mock_load_manifest, mock_popen, mock_run_checked):
        mock_target = mock.Mock()
        mock_target.arch = "x86_64"
        mock_target.run.nographic = True
        mock_resolve_target.return_value = mock_target
        mock_load_manifest.return_value = {"arch": "x86_64", "run_config": {}, "artifacts": {}}

        with tempfile.TemporaryDirectory() as d:
            mock_get_manifest_dir.return_value = Path(d)
            mock_get_output_root.return_value = Path(d)

            mock_process = mock.Mock()
            mock_popen.return_value = mock_process

            serial_log = Path(d) / "artifacts/bootstrap-smoke/serial.log"
            serial_log.parent.mkdir(parents=True, exist_ok=True)

            out_of_order = [
                "USER_INIT: ENTERED",
                "NAMESVC_MAIN_ENTER",
                "PROCESS_MANAGER_LAUNCH",
                "PROCESS_MANAGER_READY",
                "BOOT_CLASS_CORE_READY",
            ]

            def mock_poll():
                serial_log.write_text("\n".join(out_of_order))
                return None
            mock_process.poll.side_effect = mock_poll

            args = argparse.Namespace(target=Path("dummy.yaml"), artifact_dir=Path(d)/"artifacts/bootstrap-smoke", timeout=0.1) # short timeout

            with self.assertRaisesRegex(BootstrapSmokeError, "timed out"):
                run_smoke(args)

if __name__ == "__main__":
    unittest.main()
