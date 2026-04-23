add_library(bharat_librbtree STATIC rbtree.c)
target_include_directories(bharat_librbtree PUBLIC ../include ../../kernel/include)
target_include_directories(bharat_librbtree INTERFACE ../include ../../kernel/include)
target_link_libraries(bharat_librbtree PUBLIC bharat_freestanding)

add_library(bharat_librbtree_obj OBJECT rbtree.c)
target_include_directories(bharat_librbtree_obj PUBLIC ../include ../../kernel/include)
target_link_libraries(bharat_librbtree_obj PRIVATE bharat_freestanding)

bharat_configure_userspace_library(bharat_librbtree)
