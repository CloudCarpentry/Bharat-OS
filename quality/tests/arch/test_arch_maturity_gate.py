import unittest
import subprocess
import os
import tempfile

class TestArchMaturityGate(unittest.TestCase):
    def setUp(self):
        self.script_path = "tools/check_arch_maturity.py"

    def run_checker(self, yaml_content):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
            f.write(yaml_content)
            temp_path = f.name

        try:
            result = subprocess.run(
                ["python3", self.script_path, temp_path],
                capture_output=True,
                text=True
            )
            return result
        finally:
            if os.path.exists(temp_path):
                os.remove(temp_path)

    def test_tier1_must_be_mmu_full(self):
        content = """
x86_64:
  tier: full
  memory_model: MMU_LITE
  boot: baseline
  trap: baseline
  syscall: baseline
  mmu: baseline
  smp: baseline
  production_candidate: true
"""
        result = self.run_checker(content)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("expected MMU_FULL", result.stdout)

    def test_production_candidate_cannot_be_none(self):
        content = """
x86_64:
  tier: full
  memory_model: NONE
  boot: baseline
  trap: baseline
  syscall: baseline
  mmu: baseline
  smp: baseline
  production_candidate: true
"""
        result = self.run_checker(content)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("memory_model='NONE', but production_candidate=true", result.stdout)

    def test_production_candidate_cannot_be_stub(self):
        content = """
x86_64:
  tier: full
  memory_model: MMU_FULL
  boot: baseline
  trap: stub
  syscall: baseline
  mmu: baseline
  smp: baseline
  production_candidate: true
"""
        result = self.run_checker(content)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("trap is 'stub', but production_candidate=true", result.stdout)

    def test_valid_tier1_passes(self):
        content = """
x86_64:
  tier: full
  memory_model: MMU_FULL
  boot: baseline
  trap: baseline
  syscall: baseline
  mmu: baseline
  smp: baseline
  production_candidate: true
arm64:
  tier: full
  memory_model: MMU_FULL
  boot: baseline
  trap: baseline
  syscall: baseline
  mmu: baseline
  smp: baseline
  production_candidate: true
riscv64:
  tier: full
  memory_model: MMU_FULL
  boot: baseline
  trap: baseline
  syscall: baseline
  mmu: baseline
  smp: baseline
  production_candidate: true
arm32:
  tier: edge32
  memory_model: MMU_LITE
  boot: partial
  trap: stub
  syscall: unsupported
  mmu: scaffold
  smp: unsupported
  production_candidate: false
riscv32:
  tier: edge32
  memory_model: MMU_LITE
  boot: partial
  trap: stub
  syscall: unsupported
  mmu: scaffold
  smp: unsupported
  production_candidate: false
"""
        result = self.run_checker(content)
        self.assertEqual(result.returncode, 0)

if __name__ == "__main__":
    unittest.main()
