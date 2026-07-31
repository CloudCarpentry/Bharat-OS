#!/usr/bin/env python3
import os
import sys
import shutil
import tempfile
import subprocess
import signal

def run_cmd(cmd, cwd=None, msg="Running command", timeout=60):
    print(f"--> [Phase: {msg}]: {' '.join(cmd)}")

    preexec = os.setsid if os.name == 'posix' else None

    p = subprocess.Popen(
        cmd,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        preexec_fn=preexec
    )

    try:
        stdout, stderr = p.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        print(f"[-] TIMEOUT of {timeout}s expired during phase '{msg}'!")
        if os.name == 'posix':
            try:
                os.killpg(os.getpgid(p.pid), signal.SIGKILL)
            except Exception:
                pass
        else:
            p.kill()
        stdout, stderr = p.communicate()
        print(f"[-] Subprocess killed. Command was: {' '.join(cmd)}")
        print(f"STDOUT:\n{stdout}\nSTDERR:\n{stderr}")
        raise RuntimeError(f"Command timed out during phase '{msg}'")

    if p.returncode != 0:
        print(f"[-] Command failed with exit code {p.returncode}")
        print(f"STDOUT:\n{stdout}\nSTDERR:\n{stderr}")
        raise RuntimeError(f"Command failed during phase '{msg}'")

    return stdout

def test_relocation():
    # Directories
    base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    mock_uapi_src = os.path.join(base_dir, "tests", "fixtures", "mock-uapi")
    consumer_src = os.path.join(base_dir, "examples", "consumer_cmake")

    print(f"Base Directory: {base_dir}")
    print(f"Mock UAPI Src: {mock_uapi_src}")
    print(f"Consumer Src: {consumer_src}")

    # Create temporary directories (Phase 1: temporary workspace creation)
    print("[1] Phase: Temporary workspace creation")
    temp_root = tempfile.mkdtemp()
    try:
        relocated_libc = os.path.join(temp_root, "bharatlibc")
        relocated_uapi = os.path.join(temp_root, "mock-uapi")
        relocated_consumer = os.path.join(temp_root, "consumer_cmake")
        build_dir = os.path.join(temp_root, "build-libc")
        install_prefix = os.path.join(temp_root, "install-prefix")
        consumer_build = os.path.join(temp_root, "build-consumer")

        # Phase 2: source copy
        print("[2] Phase: Source copy")
        shutil.copytree(base_dir, relocated_libc)
        shutil.copytree(consumer_src, relocated_consumer)

        # Phase 3: UAPI fixture preparation
        print("[3] Phase: UAPI fixture preparation")
        shutil.copytree(mock_uapi_src, relocated_uapi)

        # Phase 4: CMake configure
        print("[4] Phase: CMake configure")
        cmake_configure_cmd = [
            "cmake",
            "-S", relocated_libc,
            "-B", build_dir,
            "-DBHARATLIBC_BACKEND=HOST",
            "-DBHARATLIBC_PROFILE=host_test",
            "-DBHARATLIBC_BUILD_TESTS=ON",
            "-DBHARATLIBC_ENABLE_META_TESTS=OFF", # CRITICAL recursion prevention guard!
            "-DBHARATLIBC_UAPI_ROOT=" + os.path.join(relocated_uapi, "include"),
            "-DCMAKE_INSTALL_PREFIX=" + install_prefix,
            "-DCMAKE_BUILD_TYPE=Debug"
        ]
        run_cmd(cmake_configure_cmd, msg="CMake configure", timeout=30)

        # Phase 5: build
        print("[5] Phase: Build")
        run_cmd(["cmake", "--build", build_dir], msg="CMake build", timeout=45)

        # Phase 6: CTest
        print("[6] Phase: CTest")
        run_cmd(["ctest", "--test-dir", build_dir, "--output-on-failure"], msg="CTest run", timeout=30)

        # Phase 7: install
        print("[7] Phase: Install")
        run_cmd(["cmake", "--install", build_dir], msg="CMake install", timeout=20)

        # Phase 8: external consumer configure
        print("[8] Phase: External consumer configure")
        cmake_consumer_configure = [
            "cmake",
            "-S", relocated_consumer,
            "-B", consumer_build,
            "-DCMAKE_PREFIX_PATH=" + install_prefix,
            "-DCMAKE_BUILD_TYPE=Debug"
        ]
        run_cmd(cmake_consumer_configure, msg="Consumer configure", timeout=20)

        # Phase 9: consumer build
        print("[9] Phase: Consumer build")
        run_cmd(["cmake", "--build", consumer_build], msg="Consumer build", timeout=20)

        # Phase 10: consumer execution
        print("[10] Phase: Consumer execution")
        consumer_exe = os.path.join(consumer_build, "consumer")
        output = run_cmd([consumer_exe], msg="Execute consumer", timeout=15)

        print("Consumer stdout:")
        print(output)

        # Verify output markers
        assert "Consumer successfully running on BharatLibC version:" in output
        assert "Relocation success!" in output
        print("[+] Consumer executed and output validated successfully!")

        # Phase 11: path-leak scan
        print("[11] Phase: Path-leak scan")
        violation_found = False
        for root, _, files in os.walk(consumer_build):
            for f in files:
                if f.endswith((".d", "Makefile", "flags.make", "link.txt", "DependInfo.cmake")):
                    filepath = os.path.join(root, f)
                    with open(filepath, "r", errors="ignore") as file_handle:
                        content = file_handle.read()
                        if "/app/core/lib/bharatlibc" in content:
                            print(f"[-] Leak detected in consumer build metadata file: {filepath}")
                            violation_found = True

        if violation_found:
            raise RuntimeError("Leak detected: Consumer build depends on original repository paths!")
        print("[+] No leaks detected. Perfect isolation proven!")

    finally:
        # Phase 12: cleanup
        print("[12] Phase: Cleanup")
        try:
            shutil.rmtree(temp_root)
        except Exception:
            pass

if __name__ == "__main__":
    try:
        test_relocation()
        print("[SUCCESS] Standalone relocation test passed perfectly!")
    except Exception as e:
        print(f"[FAILURE] Relocation test failed: {e}")
        sys.exit(1)
