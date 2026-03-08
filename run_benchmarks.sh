#!/bin/bash
mkdir -p build-tests
cd build-tests
cmake ../tests
make
ctest -R test_bench_sched -V
