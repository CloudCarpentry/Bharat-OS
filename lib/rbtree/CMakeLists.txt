add_library(bharat_librbtree STATIC rbtree.c)
target_include_directories(bharat_librbtree PUBLIC ../include)
target_include_directories(bharat_librbtree INTERFACE ../include)
target_link_libraries(bharat_librbtree PUBLIC bharat_freestanding)

add_library(bharat_librbtree_obj OBJECT rbtree.c)
target_include_directories(bharat_librbtree_obj PUBLIC ../include)
target_compile_options(bharat_librbtree_obj PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-ffreestanding>
    $<$<COMPILE_LANGUAGE:C>:-fno-builtin>
)
bharat_configure_userspace_library(bharat_librbtree)
