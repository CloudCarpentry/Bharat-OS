
set(DS_TESTS
    test_bh_id_allocator
    test_bh_range_tree
    test_bh_ring
    test_bh_seqlock
    test_bh_rcu_stub
    test_bh_list
    test_bh_bitmap
    test_bh_handle_table
    test_bh_cpumask
    test_bh_mpsc_queue
    test_bh_registry
)

foreach(test_name IN LISTS DS_TESTS)
    set(src "")
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
    elseif(test_name STREQUAL "test_bh_bitmap")
        set(src bh_bitmap.c)
    elseif(test_name STREQUAL "test_bh_handle_table")
        set(src bh_handle_table.c)
    elseif(test_name STREQUAL "test_bh_cpumask")
        set(src bh_cpumask.c)
    elseif(test_name STREQUAL "test_bh_mpsc_queue")
        set(src bh_mpsc_queue.c)
    elseif(test_name STREQUAL "test_bh_registry")
        set(src bh_registry.c)
    endif()

    if(src STREQUAL "")
        add_executable(${test_name} ${test_name}.c
            ${CMAKE_SOURCE_DIR}/core/kernel/src/lib/base/status.c
        )
    else()
        add_executable(${test_name} ${test_name}.c
            ${CMAKE_SOURCE_DIR}/core/kernel/src/ds/${src}
            ${CMAKE_SOURCE_DIR}/core/kernel/src/lib/base/status.c
        )
    endif()

    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_SOURCE_DIR}/interface/include
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
