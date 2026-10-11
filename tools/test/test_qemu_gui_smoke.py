import json
import socket
import subprocess
import tempfile
import threading
import time
import unittest
from pathlib import Path
from unittest import mock

from tools.test.qemu_gui_smoke import (
    GuiSmokeError,
    IncrementalLogReader,
    PpmImage,
    QmpClient,
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

    def test_rejects_zero_or_negative_dimensions(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.ppm"
            path.write_bytes(b"P6\n-1 1\n255\n" + bytes((0, 1, 2)))
            with self.assertRaisesRegex(GuiSmokeError, "zero or negative"):
                read_ppm(path)
            path.write_bytes(b"P6\n1 0\n255\n" + bytes((0, 1, 2)))
            with self.assertRaisesRegex(GuiSmokeError, "zero or negative"):
                read_ppm(path)

    def test_rejects_excessively_large_dimensions(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.ppm"
            path.write_bytes(b"P6\n16385 1\n255\n" + bytes((0, 1, 2)))
            with self.assertRaisesRegex(GuiSmokeError, "dimensions exceed"):
                read_ppm(path)

    def test_rejects_incorrect_max_color_value(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.ppm"
            path.write_bytes(b"P6\n1 1\n254\n" + bytes((0, 1, 2)))
            with self.assertRaisesRegex(GuiSmokeError, "invalid max value"):
                read_ppm(path)

    def test_rejects_truncated_pixel_data(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.ppm"
            path.write_bytes(b"P6\n1 1\n255\n" + bytes((0, 1)))
            with self.assertRaisesRegex(GuiSmokeError, "truncated pixel data"):
                read_ppm(path)

    def test_rejects_extra_pixel_data(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.ppm"
            path.write_bytes(b"P6\n1 1\n255\n" + bytes((0, 1, 2, 3)))
            with self.assertRaisesRegex(GuiSmokeError, "extra pixel data"):
                read_ppm(path)

    def test_rejects_malformed_header_unexpected_eof(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.ppm"
            path.write_bytes(b"P6\n1 1")
            with self.assertRaisesRegex(GuiSmokeError, "unexpected end of header"):
                read_ppm(path)

    def test_rejects_header_token_too_long(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.ppm"
            path.write_bytes(b"P6\n" + b"1" * 129 + b"\n1\n255\n" + bytes((0, 1, 2)))
            with self.assertRaisesRegex(GuiSmokeError, "header token too long"):
                read_ppm(path)

    def test_rejects_header_comment_too_long(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.ppm"
            path.write_bytes(b"P6\n#" + b"a" * 1024 + b"\n1 1\n255\n" + bytes((0, 1, 2)))
            with self.assertRaisesRegex(GuiSmokeError, "comment too long"):
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


class QmpClientTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.sock_path = Path(self.temp_dir.name) / "qmp.sock"
        self.server_thread = None
        self.server_socket = None

    def tearDown(self):
        if self.server_socket:
            self.server_socket.close()
        if self.server_thread:
            self.server_thread.join(timeout=2.0)
        self.temp_dir.cleanup()

    def _run_server(self, handler):
        self.server_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.server_socket.bind(str(self.sock_path))
        self.server_socket.listen(1)
        self.server_socket.settimeout(2.0)

        def serve():
            try:
                conn, _ = self.server_socket.accept()
                with conn, conn.makefile("rwb", buffering=0) as stream:
                    handler(stream)
            except Exception:
                pass

        self.server_thread = threading.Thread(target=serve)
        self.server_thread.start()

    def test_valid_greeting_and_capability_negotiation(self):
        def handler(stream):
            stream.write(b'{"QMP": {"version": {"qemu": {"micro": 0, "minor": 0, "major": 6}, "package": ""}, "capabilities": []}}\r\n')
            req = json.loads(stream.readline())
            if req.get("execute") == "qmp_capabilities":
                stream.write(b'{"return": {}}\r\n')

        self._run_server(handler)
        qmp = QmpClient(self.sock_path, time.monotonic() + 1.0)
        qmp.close()

    def test_missing_greeting(self):
        def handler(stream):
            stream.write(b'{"return": {}}\r\n')

        self._run_server(handler)
        with self.assertRaisesRegex(GuiSmokeError, "greeting was missing"):
            QmpClient(self.sock_path, time.monotonic() + 1.0)

    def test_malformed_json(self):
        def handler(stream):
            stream.write(b'{"QMP": ... invalid json ... \r\n')

        self._run_server(handler)
        with self.assertRaisesRegex(GuiSmokeError, "malformed JSON"):
            QmpClient(self.sock_path, time.monotonic() + 1.0)

    def test_asynchronous_event_before_command_response(self):
        def handler(stream):
            stream.write(b'{"QMP": {}}\r\n')
            req = json.loads(stream.readline())
            if req.get("execute") == "qmp_capabilities":
                stream.write(b'{"event": "SOME_EVENT"}\r\n')
                stream.write(b'{"return": {}}\r\n')
            req2 = json.loads(stream.readline())
            if req2.get("execute") == "test_cmd":
                stream.write(b'{"event": "ANOTHER_EVENT"}\r\n')
                stream.write(b'{"return": {"success": true}}\r\n')

        self._run_server(handler)
        qmp = QmpClient(self.sock_path, time.monotonic() + 1.0)
        res = qmp.execute("test_cmd")
        self.assertEqual(res, {"return": {"success": True}})
        qmp.close()

    def test_command_level_error_response(self):
        def handler(stream):
            stream.write(b'{"QMP": {}}\r\n')
            req = json.loads(stream.readline())
            if req.get("execute") == "qmp_capabilities":
                stream.write(b'{"return": {}}\r\n')
            req2 = json.loads(stream.readline())
            if req2.get("execute") == "bad_cmd":
                stream.write(b'{"error": {"class": "GenericError", "desc": "command not found"}}\r\n')

        self._run_server(handler)
        qmp = QmpClient(self.sock_path, time.monotonic() + 1.0)
        with self.assertRaisesRegex(GuiSmokeError, "QMP bad_cmd failed: command not found"):
            qmp.execute("bad_cmd")
        qmp.close()

    def test_connection_closed_during_request(self):
        def handler(stream):
            stream.write(b'{"QMP": {}}\r\n')
            req = json.loads(stream.readline())
            if req.get("execute") == "qmp_capabilities":
                stream.write(b'{"return": {}}\r\n')
            # The client issues next request, server closes stream
            req2 = json.loads(stream.readline())
            # implicitly close when handler ends

        self._run_server(handler)
        qmp = QmpClient(self.sock_path, time.monotonic() + 1.0)
        with self.assertRaisesRegex(GuiSmokeError, "closed unexpectedly"):
            qmp.execute("some_cmd")
        qmp.close()

    def test_connection_timeout(self):
        with self.assertRaisesRegex(GuiSmokeError, "timed out connecting to QMP"):
            QmpClient(self.sock_path, time.monotonic() + 0.1)

    def test_correct_socket_and_stream_cleanup(self):
        def handler(stream):
            stream.write(b'{"wrong": "greeting"}\r\n')

        self._run_server(handler)

        # Test that raising missing greeting cleans up sockets
        original_socket = socket.socket
        created_socket = None

        def mock_socket(*args, **kwargs):
            nonlocal created_socket
            created_socket = original_socket(*args, **kwargs)
            return created_socket

        with mock.patch("socket.socket", side_effect=mock_socket):
            with self.assertRaisesRegex(GuiSmokeError, "greeting was missing"):
                QmpClient(self.sock_path, time.monotonic() + 1.0)

        # If QmpClient cleans up correctly, the socket should be closed
        self.assertTrue(created_socket.fileno() == -1)

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


class IncrementalLogReaderTests(unittest.TestCase):
    def test_log_file_initially_absent(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "serial.log"
            reader = IncrementalLogReader(path)
            self.assertEqual(reader.read_new_content(), "")
            reader.close()

    def test_file_created_after_initialization(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "serial.log"
            reader = IncrementalLogReader(path)
            self.assertEqual(reader.read_new_content(), "")

            path.write_text("hello world")
            self.assertEqual(reader.read_new_content(), "hello world")
            reader.close()

    def test_incremental_appends(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "serial.log"
            path.write_text("first\n")
            reader = IncrementalLogReader(path)
            self.assertEqual(reader.read_new_content(), "first\n")

            with path.open("a") as f:
                f.write("second\n")
            self.assertEqual(reader.read_new_content(), "first\nsecond\n")
            reader.close()

    def test_marker_split_across_two_writes(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "serial.log"
            path.write_text("first part of ")
            reader = IncrementalLogReader(path)
            self.assertEqual(reader.read_new_content(), "first part of ")

            with path.open("a") as f:
                f.write("a marker")
            self.assertEqual(reader.read_new_content(), "first part of a marker")
            reader.close()

    def test_repeated_reads_with_no_new_data(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "serial.log"
            path.write_text("data\n")
            reader = IncrementalLogReader(path)
            self.assertEqual(reader.read_new_content(), "data\n")
            self.assertEqual(reader.read_new_content(), "data\n")
            reader.close()

    def test_overlap_buffer_behavior(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "serial.log"
            path.write_text("a" * 300)
            reader = IncrementalLogReader(path)

            content = reader.read_new_content(overlap=10)
            self.assertEqual(content, "a" * 300)

            with path.open("a") as f:
                f.write("b" * 5)
            self.assertEqual(reader.read_new_content(overlap=10), "a" * 10 + "b" * 5)
            reader.close()

    def test_proper_file_closure(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "serial.log"
            path.write_text("data")
            reader = IncrementalLogReader(path)
            reader.read_new_content()
            self.assertIsNotNone(reader.file)
            self.assertFalse(reader.file.closed)
            file_obj = reader.file
            reader.close()
            self.assertIsNone(reader.file)
            self.assertTrue(file_obj.closed)


if __name__ == "__main__":
    unittest.main()
