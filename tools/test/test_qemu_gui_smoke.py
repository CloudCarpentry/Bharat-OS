import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from tools.test.qemu_gui_smoke import (
    GuiSmokeError,
    PpmImage,
    make_qemu_command,
    read_ppm,
    stop_qemu,
    validate_frame,
    validate_frame_change,
    wait_for_input_marker,
    wait_for_markers,
)


class PpmTests(unittest.TestCase):
    def test_reads_commented_ppm(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.ppm"
            path.write_bytes(b"P6\n# qemu\n2 1\n255\n" + bytes((0, 1, 2, 3, 4, 5)))
            image = read_ppm(path)
        self.assertEqual((image.width, image.height), (2, 1))
        self.assertEqual(image.pixels, bytes((0, 1, 2, 3, 4, 5)))

    def test_rejects_malformed_ppm(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.ppm"
            path.write_bytes(b"P3\n1 1\n255\n0 0 0")
            with self.assertRaisesRegex(GuiSmokeError, "expected P6"):
                read_ppm(path)

    def test_rejects_wrong_dimensions(self):
        image = PpmImage(2, 2, bytes(12))
        with self.assertRaisesRegex(GuiSmokeError, "do not match guest mode"):
            validate_frame(image, 3, 2)

    def test_rejects_uniform_frame(self):
        image = PpmImage(10, 10, bytes((12, 34, 56)) * 100)
        with self.assertRaisesRegex(GuiSmokeError, "blank or uniform"):
            validate_frame(image, 10, 10)

    def test_accepts_diverse_frame(self):
        pixels = b"".join(bytes((index, index, index)) for index in range(100))
        colors, ratio = validate_frame(PpmImage(10, 10, pixels), 10, 10)
        self.assertEqual(colors, 100)
        self.assertGreater(ratio, 0.9)

    def test_rejects_unchanged_interaction_frame(self):
        image = PpmImage(10, 10, bytes((12, 34, 56)) * 100)
        with self.assertRaisesRegex(GuiSmokeError, "no visible change"):
            validate_frame_change(image, image)

    def test_accepts_bounded_interaction_change(self):
        before = PpmImage(10, 10, bytes(300))
        after = PpmImage(10, 10, bytes((255, 0, 0)) + bytes(297))
        self.assertEqual(
            validate_frame_change(before, after, minimum_changed_ratio=0.01),
            0.01,
        )

    def test_rejects_whole_frame_interaction_change(self):
        before = PpmImage(10, 10, bytes(300))
        after = PpmImage(10, 10, bytes((255, 255, 255)) * 100)
        with self.assertRaisesRegex(GuiSmokeError, "too much"):
            validate_frame_change(before, after)


class HarnessTests(unittest.TestCase):
    @mock.patch("tools.test.qemu_gui_smoke.build_qemu_command")
    def test_command_keeps_display_device_and_adds_private_channels(self, build_command):
        build_command.return_value = [
            "qemu-system-x86_64",
            "-device",
            "virtio-gpu-pci",
            "-nographic",
            "-serial",
            "stdio",
        ]
        command = make_qemu_command({}, Path("serial.log"), Path("qmp.sock"))
        self.assertIn("virtio-gpu-pci", command)
        self.assertNotIn("-nographic", command)
        self.assertIn("file:serial.log", command)
        self.assertIn("unix:qmp.sock,server=on,wait=off", command)

    def test_marker_timeout_is_bounded(self):
        process = mock.Mock()
        process.poll.return_value = None
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "serial.log"
            with self.assertRaisesRegex(GuiSmokeError, "timed out"):
                wait_for_markers(log, process, 0.0)

    def test_marker_parser_returns_queried_mode(self):
        process = mock.Mock()
        process.poll.return_value = None
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "serial.log"
            log.write_text(
                "[gui] display-ready width=1024 height=768 refresh=60\n"
                "[gui] first-frame-presented frame=1\n"
            )
            self.assertEqual(wait_for_markers(log, process, 1e20), (1024, 768))

    def test_input_marker_is_required(self):
        process = mock.Mock()
        process.poll.return_value = None
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "serial.log"
            log.write_text("[gui] first-frame-presented frame=1\n")
            with self.assertRaisesRegex(GuiSmokeError, "input-observed"):
                wait_for_input_marker(log, process, 0.0)

    def test_input_marker_is_observed(self):
        process = mock.Mock()
        process.poll.return_value = None
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "serial.log"
            log.write_text("[gui] input-observed\n")
            wait_for_input_marker(log, process, 1e20)

    def test_forced_termination_after_qmp_failure(self):
        process = mock.Mock()
        process.poll.return_value = None
        process.wait.side_effect = [subprocess.TimeoutExpired("qemu", 3), None]
        qmp = mock.Mock()
        qmp.execute.side_effect = GuiSmokeError("closed")
        self.assertTrue(stop_qemu(process, qmp))
        process.terminate.assert_called_once()


if __name__ == "__main__":
    unittest.main()
