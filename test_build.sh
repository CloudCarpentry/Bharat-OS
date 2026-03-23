#!/bin/bash
# A dummy script to see if tests compile
cmake --preset host-test && cmake --build --preset host-test
