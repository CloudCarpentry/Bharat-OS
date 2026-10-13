1. **Define filesystem operations contract (`core/lib/fs/fs_client_ops.h`)**
    - Use `write_file` to create `core/lib/fs/fs_client_ops.h` with a struct `fs_client_ops_t` containing function pointers for `open`, `openat`, `read`, `write`, `close`, `mount`, `lseek`, `fstat`, `mkdir`, `readdir`, `unlink`.
    - Provide `void fs_client_set_ops(const fs_client_ops_t* ops);` in this header.

2. **Refactor `core/lib/fs/fs_client.c`**
    - Modify `core/lib/fs/fs_client.c` using `write_file` or `replace_with_git_merge_diff`.
    - Remove all direct references to `fsd_*` and `vfs_*` functions.
    - Implement the `fs_*` functions to forward calls through the registered `fs_client_ops_t` adapter.
    - If no adapter is registered, return `-ENOSYS`.
    - Implement `fs_client_set_ops()`.

3. **Implement Direct Adapter (`core/lib/fs/adapter/fs_adapter_direct.c`)**
    - Use `write_file` to create `core/lib/fs/adapter/fs_adapter_direct.c`.
    - This adapter will contain the direct extern references to `fsd_*` and `vfs_*` functions.
    - Provide `void fs_adapter_direct_init(void)` which calls `fs_client_set_ops()` with its direct implementation.

4. **Implement Mock IPC Adapter (`core/lib/fs/adapter/fs_adapter_mock_ipc.c`)**
    - Use `write_file` to create `core/lib/fs/adapter/fs_adapter_mock_ipc.c`.
    - Create a mock adapter to prove replaceability and simulate IPC behavior.
    - Provide `void fs_adapter_mock_ipc_init(void)`.

5. **Update Build System (`core/lib/CMakeLists.txt`)**
    - Use `replace_with_git_merge_diff` to modify `core/lib/CMakeLists.txt` to include `add_subdirectory(fs)`.
    - Use `write_file` to create a new `core/lib/fs/CMakeLists.txt` defining a `bharat_libfs_client` target that includes `fs_client.c`, `adapter/fs_adapter_direct.c`, and `adapter/fs_adapter_mock_ipc.c`.
    - Modify `core/lib/CMakeLists.txt` to link `bharat_libfs_client` to `bharatsys`. Wait, `bharatsys` might not link `bharat_libfs_client` automatically if it's static. Let's just modify `core/lib/CMakeLists.txt` to add `fs_client.c`, `fs/adapter/fs_adapter_direct.c`, and `fs/adapter/fs_adapter_mock_ipc.c` directly to the `bharatsys` target source list if there is one, or build a new library `bharat_libfs` and link it where `fs_client.c` was being used. (Actually, `fs_client.c` was mysteriously absent from `CMakeLists.txt`, so I will ensure it is built by creating `bharat_libfs_client` and adding it).

6. **Verify Source Changes**
    - Use `read_file` to verify the modifications to `core/lib/fs/fs_client.c`, `core/lib/CMakeLists.txt`, and the newly created files (`fs_client_ops.h`, `fs_adapter_direct.c`, `fs_adapter_mock_ipc.c`, `core/lib/fs/CMakeLists.txt`).

7. **Create Unit Tests (`quality/tests/host/fs/test_fs_client.c`)**
    - Use `write_file` to create `quality/tests/host/fs/test_fs_client.c`.
    - The tests will cover: `test_fs_client_missing_transport()`, `test_fs_client_direct_adapter()`, `test_fs_client_mock_ipc_adapter()`, testing parameters and return values forwarding.
    - Update `quality/tests/host/CMakeLists.txt` to build and register `test_fs_client` using `replace_with_git_merge_diff`.

8. **Verify the new test file**
    - Use `read_file` to verify the content of `quality/tests/host/fs/test_fs_client.c` and `quality/tests/host/CMakeLists.txt`.

9. **Build and Run Tests**
    - Run standard build and tests using the discovered build CLI `python3 tools/build.py all --target x86_64_desktop_headless` and also explicitly test using standard cmake (`cmake -B build && cmake --build build && ctest --test-dir build`).

10. **Pre-commit Steps**
    - Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.

11. **Submit**
    - Commit and push changes.
