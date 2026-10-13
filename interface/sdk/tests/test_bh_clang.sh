#!/usr/bin/env bash
set -e

SDK_ROOT=$(cd "$(dirname "$0")/.." && pwd)
BH_CLANG="$SDK_ROOT/bin/bh-clang"
TEST_WORK_DIR=$(mktemp -d)

# Cleanup on exit
trap 'rm -rf "$TEST_WORK_DIR"' EXIT

echo "int main() { return 0; }" > "$TEST_WORK_DIR/main.c"
echo "int foo() { return 1; }" > "$TEST_WORK_DIR/foo.c"
echo "#include <stdio.h>" > "$TEST_WORK_DIR/pre.c"

# Mock compiler wrapper for checking arguments
cat << 'MOCK' > "$TEST_WORK_DIR/mock_cc"
#!/usr/bin/env bash
echo "$@"
MOCK
chmod +x "$TEST_WORK_DIR/mock_cc"

# Mock SDK libraries for testing
MOCK_BUILD_DIR="$TEST_WORK_DIR/build-mock"
mkdir -p "$MOCK_BUILD_DIR"
touch "$MOCK_BUILD_DIR/libbharat_core.a"
touch "$MOCK_BUILD_DIR/libbharat_process.a"
touch "$MOCK_BUILD_DIR/libbharat_ipc.a"
touch "$MOCK_BUILD_DIR/libbharat_device.a"
touch "$MOCK_BUILD_DIR/libbharat_cap.a"
touch "$MOCK_BUILD_DIR/libbharat_compute.a"

export BHARAT_SDK_CC="$TEST_WORK_DIR/mock_cc"
export BHARAT_SDK_BUILD_DIR="$MOCK_BUILD_DIR"

echo "Test 1: Compile-only mode (-c)"
out=$("$BH_CLANG" -c "$TEST_WORK_DIR/main.c")
if echo "$out" | grep -q "libbharat"; then
    echo "FAIL: Libraries linked in compile-only mode"
    exit 1
fi
if ! echo "$out" | grep -q "\-c"; then
    echo "FAIL: Missing -c flag"
    exit 1
fi

echo "Test 2: Preprocessing (-E)"
out=$("$BH_CLANG" -E "$TEST_WORK_DIR/pre.c")
if echo "$out" | grep -q "libbharat"; then
    echo "FAIL: Libraries linked in preprocess mode"
    exit 1
fi

echo "Test 3: Multiple source files"
out=$("$BH_CLANG" "$TEST_WORK_DIR/main.c" "$TEST_WORK_DIR/foo.c")
if ! echo "$out" | grep -q "libbharat_core.a"; then
    echo "FAIL: Missing core library in link mode"
    exit 1
fi

echo "Test 4: Paths containing spaces"
mkdir -p "$TEST_WORK_DIR/spaced dir"
echo "int main() { return 0; }" > "$TEST_WORK_DIR/spaced dir/main.c"
out=$("$BH_CLANG" -c "$TEST_WORK_DIR/spaced dir/main.c")
if ! echo "$out" | grep -q "spaced dir/main.c"; then
    echo "FAIL: Space not preserved in path"
    exit 1
fi

echo "Test 5: Unknown backend"
if "$BH_CLANG" --bh-backend=UNKNOWN >/dev/null 2>&1; then
    echo "FAIL: Did not reject unknown backend"
    exit 1
fi

echo "Test 6: Unknown architecture"
if "$BH_CLANG" --bh-backend=NATIVE --bh-arch=unknown_arch >/dev/null 2>&1; then
    echo "FAIL: Did not reject unknown architecture"
    exit 1
fi

echo "Test 7: Missing native runtime libs"
rm "$MOCK_BUILD_DIR/libbharat_process.a"
if "$BH_CLANG" --bh-backend=NATIVE --bh-arch=x86_64 >/dev/null 2>&1; then
    echo "FAIL: Did not fail on missing native runtime libraries"
    exit 1
fi
touch "$MOCK_BUILD_DIR/libbharat_process.a"

echo "Test 8: Host build caching"
export BHARAT_SDK_BUILD_DIR=""
export BHARAT_SDK_BACKEND="HOST"
mkdir -p "$SDK_ROOT/build-host"
touch "$SDK_ROOT/build-host/libbharat_core.a"
touch "$SDK_ROOT/build-host/libbharat_process.a"
touch "$SDK_ROOT/build-host/libbharat_ipc.a"
touch "$SDK_ROOT/build-host/libbharat_device.a"
touch "$SDK_ROOT/build-host/libbharat_cap.a"

out=$("$BH_CLANG" "$TEST_WORK_DIR/main.c")
if ! echo "$out" | grep -q "build-host/libbharat_core.a"; then
    echo "FAIL: Did not link correct host cache libs"
    exit 1
fi

echo "Test 9: Native build caching"
export BHARAT_SDK_BACKEND="NATIVE"
export BHARAT_SDK_ARCH="x86_64"
NATIVE_BUILD_DIR="$SDK_ROOT/build/sdk/x86_64-native"
mkdir -p "$NATIVE_BUILD_DIR"
touch "$NATIVE_BUILD_DIR/libbharat_core.a"
touch "$NATIVE_BUILD_DIR/libbharat_process.a"
touch "$NATIVE_BUILD_DIR/libbharat_ipc.a"
touch "$NATIVE_BUILD_DIR/libbharat_device.a"
touch "$NATIVE_BUILD_DIR/libbharat_cap.a"

out=$("$BH_CLANG" "$TEST_WORK_DIR/main.c")
if ! echo "$out" | grep -q "build/sdk/x86_64-native/libbharat_core.a"; then
    echo "FAIL: Did not link correct native cache libs"
    exit 1
fi

echo "Test 10: Compute library requested"
out=$("$BH_CLANG" "$TEST_WORK_DIR/main.c" -lbharat_compute)
if ! echo "$out" | grep -q "libbharat_compute.a"; then
    echo "FAIL: Did not link compute library when requested"
    exit 1
fi

echo "All tests passed!"
