include(${RUNTIME_COMMON_DIR}/core/core.cmake)

set(RUNTIME_COMMON_SRC "${CORE_SRC}")

if(COMPILE_RUNTIME_LIGHT)
  set(RUNTIME_COMMON_SOURCES_FOR_COMP "${RUNTIME_COMMON_SRC}")
  configure_file(${BASE_DIR}/compiler/runtime_common_sources.h.in
                 ${AUTO_DIR}/compiler/runtime_common_sources.h)
endif()

prepend(RUNTIME_COMMON_SRC ${RUNTIME_COMMON_DIR}/ "${RUNTIME_COMMON_SRC}")
vk_add_library(runtime-common OBJECT ${RUNTIME_COMMON_SRC})

if(COMPILE_RUNTIME_LIGHT)
  target_compile_options(runtime-common PUBLIC -fPIC)
endif()
