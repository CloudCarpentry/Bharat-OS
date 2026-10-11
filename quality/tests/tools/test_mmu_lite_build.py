"""MMU-Lite needs real eager mapping even when advanced VM is disabled."""
import json
import subprocess
from pathlib import Path


def test_rtos_mmu_lite_selects_real_mapping_objects(tmp_path):
    repo = Path(__file__).resolve().parents[3]
    subprocess.run([
        "cmake", "--preset=x86_64-dev", "-B", str(tmp_path),
        "-DBHARAT_DEVICE_PROFILE=RTOS", "-DBHARAT_BOOT_GUI=OFF",
        "-DBHARAT_PROFILE_MMU_FULL=OFF", "-DBHARAT_PROFILE_MMU_LITE=ON",
        "-DBHARAT_ENABLE_ADVANCED_VM=OFF", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ], cwd=repo, check=True, capture_output=True)
    compiled = {Path(item["file"]).relative_to(repo).as_posix()
                for item in json.loads((tmp_path / "compile_commands.json").read_text())
                if Path(item["file"]).is_relative_to(repo)}
    assert "core/kernel/src/mm/vmm.c" in compiled
    assert "core/kernel/src/mm/vm/aspace/aspace.c" in compiled
    assert "core/kernel/src/mm/vmm_stub.c" not in compiled
    config = json.loads((tmp_path / "generated/build-configuration.json").read_text())
    assert config["functional"]["memory_advanced_vm"] == "OFF"
    header = (tmp_path / "generated/include/bharat_config.h").read_text()
    assert "#define BHARAT_ENABLE_COW" not in header
    assert "#define BHARAT_ENABLE_DEMAND_PAGING" not in header
