add_library(bharat_stacks_storage STATIC
    block/block.c
    cache/cache.c
    profile.c
    fs/adapter.c
    fs/ramfs/ramfs.c
    fs/blob/blob_backend.c
)
target_include_directories(bharat_stacks_storage PUBLIC ../include)
target_include_directories(bharat_stacks_storage PRIVATE ../../kernel/include)
target_include_directories(bharat_stacks_storage PRIVATE ../../include)
target_link_libraries(bharat_stacks_storage PRIVATE bharat_user_buildopts bharat_freestanding)
