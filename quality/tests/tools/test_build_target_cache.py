"""A target sharing a preset must not inherit the previous target's options."""

import json
import subprocess
from types import SimpleNamespace

from tools.build.build_executor import make_build_plan
from tools.build.models import BuildConfig


def test_shared_preset_resets_omitted_target_options(tmp_path):
    (tmp_path / "CMakeLists.txt").write_text(
        'cmake_minimum_required(VERSION 3.20)\n'
        'project(TargetCache NONE)\n'
        'option(BHARAT_INIT_CORE_BOOTSTRAP_ONLY "Test graph selection" OFF)\n'
        'file(WRITE "${CMAKE_BINARY_DIR}/selection.txt" '
        '"${BHARAT_INIT_CORE_BOOTSTRAP_ONLY},${BHARAT_DEVICE_PROFILE}")\n'
    )
    (tmp_path / "CMakePresets.json").write_text(json.dumps({
        "version": 3,
        "configurePresets": [{
            "name": "shared", "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/shared",
            "cacheVariables": {"BHARAT_DEVICE_PROFILE": "DESKTOP"},
        }],
    }))

    for expected, definitions in [
        ("ON,DESKTOP", {"BHARAT_INIT_CORE_BOOTSTRAP_ONLY": "ON"}),
        ("OFF,DESKTOP", {}),
    ]:
        target = SimpleNamespace(build=BuildConfig("shared", definitions))
        plan = make_build_plan(target, tmp_path)
        for command in plan.command_summary[:-1]:
            subprocess.run(command, cwd=tmp_path, check=True, capture_output=True)
        assert (tmp_path / "build/shared/selection.txt").read_text() == expected
