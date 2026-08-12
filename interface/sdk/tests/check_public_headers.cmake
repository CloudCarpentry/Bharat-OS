file(REMOVE_RECURSE "${BUILD_DIR}")
file(MAKE_DIRECTORY "${BUILD_DIR}")
file(GLOB_RECURSE headers "${SDK_SOURCE}/include/bharat/*.h")
foreach(header IN LISTS headers)
    file(RELATIVE_PATH relative "${SDK_SOURCE}/include" "${header}")
    string(REPLACE "/" "_" name "${relative}")
    file(WRITE "${BUILD_DIR}/${name}.c" "#include <${relative}>\nint main(void) { return 0; }\n")
    execute_process(COMMAND "${C_COMPILER}" -std=c11 -Wall -Wextra -Werror
        -I "${SDK_SOURCE}/include" -I "${SDK_SOURCE}/../uapi" -I "${SDK_SOURCE}/../include"
        -c "${BUILD_DIR}/${name}.c" -o "${BUILD_DIR}/${name}.o"
        RESULT_VARIABLE result ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Public header ${name}.h is not standalone:\n${error}")
    endif()
endforeach()
