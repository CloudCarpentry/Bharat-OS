#!/usr/bin/env python3
import copy
import importlib.util
import json
import pathlib
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL_PATH = REPO_ROOT / "tools/abi/syscall_abi.py"
SPEC = importlib.util.spec_from_file_location("syscall_abi", TOOL_PATH)
syscall_abi = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(syscall_abi)


class SyscallAbiTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(
            (REPO_ROOT / "interface/contracts/abi/native_syscalls.json").read_text()
        )
        cls.lock = json.loads(
            (REPO_ROOT / "interface/contracts/abi/native_syscalls.lock.json").read_text()
        )

    def test_current_manifest_lock_and_generated_outputs_validate(self):
        self.assertTrue(syscall_abi.validate_schema(self.manifest))
        self.assertTrue(syscall_abi.validate_semantics(self.manifest))
        self.assertTrue(syscall_abi.compare_lock(self.manifest, self.lock))
        self.assertTrue(syscall_abi.verify_generated(self.manifest, self.lock))

    def test_manifest_schema_rejects_missing_handler(self):
        manifest = copy.deepcopy(self.manifest)
        del manifest["syscalls"][0]["handler"]
        self.assertFalse(syscall_abi.validate_schema(manifest))

    def test_schema_rejects_unknown_argument_metadata(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["syscalls"][1]["arguments"][0]["unchecked"] = True
        self.assertFalse(syscall_abi.validate_schema(manifest))

    def test_schema_rejects_unknown_capability_source_kind(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["syscalls"][2]["capability"]["source"]["kind"] = "ambient"
        self.assertFalse(syscall_abi.validate_schema(manifest))

    def test_schema_requires_explicit_capability_validation_phase(self):
        manifest = copy.deepcopy(self.manifest)
        del manifest["syscalls"][2]["capability"]["validation_phase"]
        self.assertFalse(syscall_abi.validate_schema(manifest))

    def test_duplicate_number_is_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["syscalls"][1]["number"] = manifest["syscalls"][0]["number"]
        self.assertFalse(syscall_abi.validate_semantics(manifest))

    def test_pointer_size_source_must_resolve(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["syscalls"][20]["arguments"][1]["size_source"] = "missing_length"
        self.assertFalse(syscall_abi.validate_semantics(manifest))

    def test_pointer_size_source_must_be_input_scalar(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["syscalls"][20]["arguments"][2]["direction"] = "out"
        self.assertFalse(syscall_abi.validate_semantics(manifest))

    def test_register_capability_source_must_be_scalar(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["syscalls"][2]["arguments"][0]["kind"] = "user_struct"
        self.assertFalse(syscall_abi.validate_semantics(manifest))

    def test_struct_capability_source_must_validate_after_usercopy(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["syscalls"][1]["capability"]["validation_phase"] = "before_handler"
        self.assertFalse(syscall_abi.validate_semantics(manifest))

    def test_unknown_trait_is_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["syscalls"][0]["traits"].append("restartable")
        self.assertFalse(syscall_abi.validate_semantics(manifest))

    def test_locked_number_change_is_rejected(self):
        manifest = copy.deepcopy(self.manifest)
        manifest["syscalls"][0]["number"] = 100
        self.assertFalse(syscall_abi.compare_lock(manifest, self.lock))

    def test_addition_requires_lock_update(self):
        manifest = copy.deepcopy(self.manifest)
        added = copy.deepcopy(manifest["syscalls"][-1])
        added.update(number=24, symbol="BH_SYS_TEST_ADDITION", name="test_addition")
        manifest["syscalls"].append(added)
        self.assertFalse(syscall_abi.compare_lock(manifest, self.lock))

    def test_metadata_count_mismatch_is_rejected(self):
        lock = copy.deepcopy(self.lock)
        lock["syscall_count"] += 1
        self.assertFalse(syscall_abi.compare_lock(self.manifest, lock))

    def test_stale_generated_output_hash_is_rejected(self):
        lock = copy.deepcopy(self.lock)
        lock["generated_sha256"]["numbers"] = "0" * 64
        self.assertFalse(syscall_abi.verify_generated(self.manifest, lock))

    def test_common_gate_routes_native_linux_and_android(self):
        gate = (REPO_ROOT / "core/kernel/src/trap/syscall_gate.c").read_text()
        self.assertEqual(gate.count("long bh_syscall_gate("), 2)  # declaration + definition
        for personality in ("NATIVE", "LINUX", "ANDROID"):
            self.assertIn(f"case BH_PERSONALITY_{personality}", gate)
        self.assertIn("personality_get_syscall_table(ctx.personality)", gate)


if __name__ == "__main__":
    unittest.main()
