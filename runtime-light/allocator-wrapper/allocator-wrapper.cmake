vk_add_library_pic(libc-alloc-wrapper-pic ${RUNTIME_LIGHT_DIR}/allocator-wrapper/libc-alloc-wrapper.cpp)
target_compile_options(libc-alloc-wrapper-pic PUBLIC -stdlib=libc++)
