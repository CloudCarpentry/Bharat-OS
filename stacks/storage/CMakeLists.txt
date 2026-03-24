add_library(bharat_stacks_storage STATIC
    block/block.c
)
target_include_directories(bharat_stacks_storage PUBLIC ../include)
target_link_libraries(bharat_stacks_storage PRIVATE bharat_user_buildopts bharat_freestanding)
