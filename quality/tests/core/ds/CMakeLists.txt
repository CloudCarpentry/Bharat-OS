
set(DS_TESTS
    test_bh_id_allocator
    test_bh_range_tree
    test_bh_ring
    test_bh_seqlock
    test_bh_rcu_stub
)

foreach(test_name IN LISTS DS_TESTS)
    if(test_name STREQUAL "test_bh_id_allocator")
        set(src bh_id_allocator.c)
    elseif(test_name STREQUAL "test_bh_range_tree")
        set(src bh_range_tree.c)
    elseif(test_name STREQUAL "test_bh_ring")
        set(src bh_ring.c)
    elseif(test_name STREQUAL "test_bh_seqlock")
        set(src bh_seqlock.c)
    elseif(test_name STREQUAL "test_bh_rcu_stub")
        set(src bh_rcu_stub.c)
    endif()

    add_executable(${test_name} ${test_name}.c
        ${CMAKE_SOURCE_DIR}/core/kernel/src/ds/${src}
        ${CMAKE_SOURCE_DIR}/core/kernel/src/lib/base/status.c
    )

    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_SOURCE_DIR}/interface/include
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
