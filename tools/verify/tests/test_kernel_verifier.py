import unittest
import json
import struct
import tempfile
from pathlib import Path
import os
import sys

# Add tools dir to path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

from verify.kernel_verifier import check_elf, ELFError, verify_manifest, get_expected_elf_class_and_machine

def make_elf(ei_class, ei_data, e_machine, e_entry, truncate=False):
    magic = b'\x7fELF'
    endian = '<' if ei_data == 1 else '>'

    # Pad e_ident to 16 bytes
    e_ident = magic + bytes([ei_class, ei_data, 1, 0, 0]) + (b'\x00' * 7)

    if ei_class == 1:
        header = struct.pack(endian + 'HHIIIIIHHHHHH', 2, e_machine, 1, e_entry, 52, 0, 0, 52, 32, 1, 40, 0, 0)
    else:
        header = struct.pack(endian + 'HHIQQQIHHHHHH', 2, e_machine, 1, e_entry, 64, 0, 0, 64, 56, 1, 64, 0, 0)

    full = e_ident + header
    full += b'\x00' * 20

    if truncate:
        return full[:40]
    return full

class TestKernelVerifier(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.dir_path = Path(self.temp_dir.name)

    def tearDown(self):
        self.temp_dir.cleanup()

    def write_file(self, name, content):
        p = self.dir_path / name
        p.write_bytes(content)
        return p

    def test_valid_elf32(self):
        p = self.write_file("test32.elf", make_elf(1, 1, 40, 0x1000))
        info = check_elf(p)
        self.assertEqual(info["class"], 1)
        self.assertEqual(info["machine"], 40)
        self.assertEqual(info["entry"], 0x1000)

    def test_valid_elf64(self):
        p = self.write_file("test64.elf", make_elf(2, 1, 62, 0x2000))
        info = check_elf(p)
        self.assertEqual(info["class"], 2)
        self.assertEqual(info["machine"], 62)
        self.assertEqual(info["entry"], 0x2000)

    def test_empty_file(self):
        p = self.write_file("empty.elf", b"")
        with self.assertRaisesRegex(ELFError, "empty"):
            check_elf(p)

    def test_invalid_magic(self):
        p = self.write_file("bad.elf", b"NOT_ELF" + b"\x00"*60)
        with self.assertRaisesRegex(ELFError, "magic"):
            check_elf(p)

    def test_unsupported_arch(self):
        p = self.write_file("bad_arch.elf", make_elf(1, 1, 999, 0))
        with self.assertRaisesRegex(ELFError, "architecture"):
            check_elf(p)

    def test_truncated_header(self):
        p = self.write_file("trunc.elf", make_elf(1, 1, 40, 0, truncate=True))
        with self.assertRaisesRegex(ELFError, "too small"):
            check_elf(p)

    def test_manifest_verification(self):
        elf_path = self.write_file("kernel.elf", make_elf(2, 1, 62, 0x1000))
        init_path = self.write_file("init.bin", b"init")

        manifest_data = {
            "arch": "x86_64",
            "artifacts": {
                "canonical_elf": "kernel.elf",
                "init_module": "init.bin"
            },
            "boot_contract": {
                "dtb": {"required": False}
            }
        }
        manifest_path = self.dir_path / "run-manifest.json"
        manifest_path.write_text(json.dumps(manifest_data))

        self.assertTrue(verify_manifest(manifest_path))

    def test_manifest_missing_elf(self):
        manifest_data = {
            "artifacts": {
                "canonical_elf": "missing.elf",
            }
        }
        manifest_path = self.dir_path / "run-manifest.json"
        manifest_path.write_text(json.dumps(manifest_data))

        self.assertFalse(verify_manifest(manifest_path))

    def test_manifest_arch_mismatch(self):
        # 32 bit ARM ELF
        elf_path = self.write_file("kernel.elf", make_elf(1, 1, 40, 0x1000))

        manifest_data = {
            "arch": "x86_64", # Expected 64 bit x86_64
            "artifacts": {
                "canonical_elf": "kernel.elf"
            }
        }
        manifest_path = self.dir_path / "run-manifest.json"
        manifest_path.write_text(json.dumps(manifest_data))

        self.assertFalse(verify_manifest(manifest_path))

if __name__ == "__main__":
    unittest.main()
