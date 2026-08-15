add_executable(bench_hmem_tensor main.c)
target_compile_features(bench_hmem_tensor PRIVATE c_std_11)
target_link_libraries(bench_hmem_tensor PRIVATE Bharat::sdk)
