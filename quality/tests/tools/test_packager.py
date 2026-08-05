from pathlib import Path

import pytest

from tools.build.models import (
    BootConfig,
    BuildConfig,
    BuildOutputs,
    DtbConfig,
    KernelConfig,
    PackageConfig,
    PackagePlan,
    ResolvedTarget,
)
from tools.package.packager import FDT_MAGIC, _compact_qemu_dtb, _find_required_service_binary, execute_package


def _dtb_bytes(total_size: int, padded_size: int) -> bytes:
    header = bytearray(40)
    header[0:4] = FDT_MAGIC.to_bytes(4, "big")
    header[4:8] = padded_size.to_bytes(4, "big")
    header[8:12] = (56).to_bytes(4, "big")
    header[12:16] = (60).to_bytes(4, "big")
    header[16:20] = (40).to_bytes(4, "big")
    header[32:36] = (total_size - 60).to_bytes(4, "big")
    header[36:40] = (4).to_bytes(4, "big")
    return bytes(header) + bytes(padded_size - len(header))


def test_compact_qemu_dtb_removes_padding(tmp_path: Path) -> None:
    dtb = tmp_path / "hw.dtb"
    dtb.write_bytes(_dtb_bytes(total_size=64, padded_size=1024))

    _compact_qemu_dtb(dtb)

    assert len(dtb.read_bytes()) == 64


@pytest.mark.parametrize(
    "contents",
    [
        b"too short",
        b"BAD!" + (40).to_bytes(4, "big") + bytes(32),
        FDT_MAGIC.to_bytes(4, "big") + (128).to_bytes(4, "big") + bytes(32),
    ],
)
def test_compact_qemu_dtb_rejects_invalid_blob(tmp_path: Path, contents: bytes) -> None:
    dtb = tmp_path / "hw.dtb"
    dtb.write_bytes(contents)

    with pytest.raises(RuntimeError, match="DTB"):
        _compact_qemu_dtb(dtb)


def test_packager_rejects_missing_required_service_payload(tmp_path: Path) -> None:
    with pytest.raises(RuntimeError, match="Required compiled payload 'services/init'.*synthetic boot module"):
        _find_required_service_binary(tmp_path, "init")


def test_packager_finds_required_service_payload(tmp_path: Path) -> None:
    payload = tmp_path / "core" / "services" / "core" / "init" / "init"
    payload.parent.mkdir(parents=True)
    payload.write_bytes(b"compiled-init")

    assert _find_required_service_binary(tmp_path, "init") == payload


def _minimal_package_plan(tmp_path: Path) -> PackagePlan:
    target = ResolvedTarget(
        name="unit_target",
        kind="qemu_target",
        arch="x86_64",
        board="qemu",
        device_profile="desktop",
        personality_profile="headless",
        execution_profile="gp",
        build=BuildConfig(cmake_preset="unit", cmake_defs={}),
        kernel=KernelConfig(canonical_elf="kernel.elf"),
        boot=BootConfig(protocol="multiboot2", artifact_format="elf", dtb=DtbConfig(mode="qemu_generated", required=False)),
        package=PackageConfig(transforms=[]),
    )
    return PackagePlan(
        target=target,
        build_outputs=BuildOutputs(target=target, build_dir=tmp_path / "build"),
        packaged_dir=tmp_path / "pkg",
        manifest_dir=tmp_path / "manifests",
    )


def test_execute_package_fails_closed_without_required_service_payload(tmp_path: Path) -> None:
    plan = _minimal_package_plan(tmp_path)
    plan.build_outputs.build_dir.mkdir()

    with pytest.raises(RuntimeError, match="Required compiled payload 'services/init'.*synthetic boot module"):
        execute_package(plan, tmp_path)

    assert not (plan.packaged_dir / "init_module.bin").exists()
