import unittest
import struct
from pathlib import Path

class TestPackageParser(unittest.TestCase):
    def test_init_module_parsing(self):
        # We will parse the generated init_module.bin if it exists.
        # This test ensures we can read the magic, offset, sizes correctly.
        target_bin = Path("build/x86_64-dev/delivery/targets/qemu/x86_64_desktop_gui/init_module.bin")
        if not target_bin.exists():
            target_bin = Path("build/x86_64-dev/packaged/init_module.bin")
        if not target_bin.exists():
            self.skipTest("init_module.bin not generated yet")

        data = target_bin.read_bytes()

        offset = 0
        modules = []
        while offset < len(data):
            if offset + 128 > len(data):
                break

            header = data[offset:offset+128]
            magic, abi_version, header_size, module_kind, payload_offset, payload_size, \
            target_arch, elf_class, flags, name_len, digest_algo, digest, name_bytes, padding = struct.unpack(
                "<IIIIIIIIIII32s32s20s", header
            )

            self.assertEqual(magic, 0xB4A2D1A5)
            self.assertEqual(header_size, 128)

            name = name_bytes[:name_len].decode("utf-8")
            modules.append((name, payload_size))

            offset += payload_offset + payload_size

        self.assertTrue(len(modules) >= 1)
        names = [m[0] for m in modules]
        self.assertIn("services/init", names)
        # Should also have namesvc and process_manager
        self.assertIn("services/namesvc", names)
        self.assertIn("services/process_manager", names)

if __name__ == '__main__':
    unittest.main()
