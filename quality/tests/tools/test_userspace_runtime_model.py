from pathlib import Path

import pytest

from tools.build.target_resolver import resolve_yaml_target, validate_yaml_target


def _target(runtime_model=None):
    target = {
        "name": "runtime_test",
        "kind": "qemu_target",
        "arch": "x86_64",
        "board": "qemu",
        "device_profile": "desktop",
        "personality_profile": "native",
        "build": {"cmake_preset": "test"},
        "kernel": {"canonical_artifacts": {"elf": "kernel.elf"}},
        "boot": {"protocol": "multiboot2", "artifact_format": "elf"},
    }
    if runtime_model is not None:
        target["userspace"] = {"runtime_model": runtime_model}
    return target


@pytest.mark.parametrize("model", ["direct", "static", "light", "full"])
def test_schema_accepts_runtime_models(model):
    validate_yaml_target(_target(model))


@pytest.mark.parametrize("userspace", [
    {"runtime_model": "managed"},
    {"runtime_model": "unknown"},
    {"runtime_model": "full", "root": "custom"},
])
def test_schema_rejects_unknown_runtime_configuration(userspace):
    target = _target()
    target["userspace"] = userspace
    with pytest.raises(SystemExit):
        validate_yaml_target(target)


def test_schema_rejects_top_level_runtime_model():
    target = _target()
    target["runtime_model"] = "direct"
    with pytest.raises(SystemExit):
        validate_yaml_target(target)


@pytest.mark.parametrize(
    "model,component,model_id",
    [("direct", "user_smoke", 0), ("static", "rt-supervisor", 1),
     ("light", "init-lite", 2), ("full", "init", 3)],
)
def test_resolver_selects_one_root(tmp_path: Path, model, component, model_id):
    import yaml
    path = tmp_path / "target.yaml"
    path.write_text(yaml.safe_dump(_target(model)))
    resolved = resolve_yaml_target(path)
    assert resolved.userspace.runtime_model == model
    assert resolved.userspace.root_component == component
    assert resolved.build.cmake_defs["BHARAT_USERSPACE_RUNTIME_MODEL_ID"] == model_id


def test_resolver_defaults_legacy_target_to_full(tmp_path: Path):
    import yaml
    path = tmp_path / "target.yaml"
    path.write_text(yaml.safe_dump(_target()))
    resolved = resolve_yaml_target(path)
    assert resolved.userspace.runtime_model == "full"
    assert resolved.userspace.root_component == "init"


@pytest.mark.parametrize("arch,model", [
    ("arm64", "direct"), ("arm64", "light"),
    ("arm32", "static"), ("riscv64", "full"),
    ("riscv32", "direct"),
])
def test_runtime_model_is_orthogonal_to_architecture(tmp_path: Path, arch, model):
    import yaml
    target = _target(model)
    target["arch"] = arch
    path = tmp_path / f"{arch}-{model}.yaml"
    path.write_text(yaml.safe_dump(target))
    resolved = resolve_yaml_target(path)
    assert resolved.arch == arch
    assert resolved.userspace.runtime_model == model
