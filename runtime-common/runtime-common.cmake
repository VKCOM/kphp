include(${RUNTIME_COMMON_DIR}/core/core.cmake)
include(${RUNTIME_COMMON_DIR}/stdlib/stdlib.cmake)
include(${THIRD_PARTY_DIR}/uber-h3-cmake/uber-h3.cmake)

set(RUNTIME_COMMON_SRC "${CORE_SRC}" "${STDLIB_SRC}")

if(COMPILE_RUNTIME_LIGHT)
  set(RUNTIME_COMMON_SOURCES_FOR_COMP "${RUNTIME_COMMON_SRC}")
  configure_file(${BASE_DIR}/compiler/runtime_common_sources.h.in
                 ${AUTO_DIR}/compiler/runtime_common_sources.h)
endif()

prepend(RUNTIME_COMMON_SRC ${RUNTIME_COMMON_DIR}/ "${RUNTIME_COMMON_SRC}")
vk_add_library_no_pic(runtime-common-no-pic OBJECT ${RUNTIME_COMMON_SRC})
vk_add_library_pic(runtime-common-pic OBJECT ${RUNTIME_COMMON_SRC})

set(RUNTIME_COMMON_LIBS_PIC
        UBER_H3::pic::uber-h3)
set(RUNTIME_COMMON_LIBS_NO_PIC
        UBER_H3::no-pic::uber-h3)

target_link_libraries(runtime-common-pic PUBLIC ${RUNTIME_COMMON_LIBS_PIC})
add_dependencies(runtime-common-pic ${RUNTIME_COMMON_LIBS_PIC})

target_link_libraries(runtime-common-no-pic PUBLIC ${RUNTIME_COMMON_LIBS_NO_PIC})
add_dependencies(runtime-common-no-pic ${RUNTIME_COMMON_LIBS_NO_PIC})

if(COMPILE_RUNTIME_LIGHT)
  target_compile_options(runtime-common-pic PUBLIC -stdlib=libc++ ${RUNTIME_LIGHT_VISIBILITY})
endif()
