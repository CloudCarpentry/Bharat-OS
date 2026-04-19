import sys

with open("lib/ipc/CMakeLists.txt", "r") as f:
    content = f.read()

content = content.replace("target_include_directories(bharat_libipc PUBLIC include)", "target_include_directories(bharat_libipc PUBLIC include ${CMAKE_SOURCE_DIR}/lib/include ${CMAKE_SOURCE_DIR}/kernel/include)")
content = content.replace("target_link_libraries(bharat_libipc PUBLIC cap bharat_freestanding)", "target_link_libraries(bharat_libipc PUBLIC cap bharat_freestanding bharat_syscall)")

with open("lib/ipc/CMakeLists.txt", "w") as f:
    f.write(content)
