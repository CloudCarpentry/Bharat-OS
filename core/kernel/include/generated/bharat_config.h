#ifndef BHARAT_CONFIG_H_SOURCE_TREE_SHIM
#define BHARAT_CONFIG_H_SOURCE_TREE_SHIM

/*
 * FAIL-FAST SHIM
 *
 * If you are seeing this error, it means your build system or IDE is including
 * the source-tree 'core/kernel/include/generated/bharat_config.h' instead of the
 * build-generated one at '${CMAKE_BINARY_DIR}/generated/include/bharat_config.h'.
 *
 * Real configuration is now build-directory only to prevent stale artifacts
 * and merge conflicts in the source tree.
 */
#error "Do not include source-tree core/kernel/include/generated/bharat_config.h. \
Use the build-generated bharat_config.h from the build directory. \
Ensure your build is configured via CMake."

#endif /* BHARAT_CONFIG_H_SOURCE_TREE_SHIM */
