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

if(COMPILE_RUNTIME_LIGHT)
  target_compile_options(runtime-common-pic PUBLIC -stdlib=libc++ ${RUNTIME_LIGHT_VISIBILITY})
endif()
