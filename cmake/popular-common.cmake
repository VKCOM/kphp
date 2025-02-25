include_guard(GLOBAL)

set(LIGHT_COMMON_SOURCES
    crc32_generic.cpp md5.cpp algorithms/simd-int-to-string.cpp
    algorithms/murmur-hash.cpp)

if(COMPILE_RUNTIME_LIGHT)
  set(COMMON_SOURCES_FOR_COMP "${LIGHT_COMMON_SOURCES}")
  configure_file(${BASE_DIR}/compiler/common_sources.h.in
                 ${AUTO_DIR}/compiler/common_sources.h)
endif()

prepend(LIGHT_COMMON_SOURCES ${COMMON_DIR}/ ${LIGHT_COMMON_SOURCES})

prepend(
  POPULAR_COMMON_SOURCES
  ${COMMON_DIR}/
  algorithms/simd-int-to-string.cpp
  resolver.cpp
  precise-time.cpp
  cpuid.cpp
  crc32_generic.cpp
  crc32.cpp
  crc32c.cpp
  options.cpp
  secure-bzero.cpp
  crc32_${CMAKE_SYSTEM_PROCESSOR}.cpp
  crc32c_${CMAKE_SYSTEM_PROCESSOR}.cpp
  version-string.cpp
  kernel-version.cpp
  kprintf.cpp
  rpc-headers.cpp
  parallel/counter.cpp
  parallel/maximum.cpp
  parallel/thread-id.cpp
  parallel/limit-counter.cpp
  server/limits.cpp
  server/signals.cpp
  server/relogin.cpp
  server/crash-dump.cpp
  server/engine-settings.cpp
  stats/buffer.cpp
  stats/provider.cpp)

if(APPLE)
  list(APPEND POPULAR_COMMON_SOURCES ${COMMON_DIR}/macos-ports.cpp)
endif()

if(COMPILE_RUNTIME_LIGHT)
  vk_add_library(light-common OBJECT ${LIGHT_COMMON_SOURCES})
  set_property(TARGET light-common PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_compile_options(light-common PUBLIC -stdlib=libc++ -fPIC)
  target_link_options(light-common PUBLIC -stdlib=libc++)
endif()

vk_add_library(popular_common OBJECT ${POPULAR_COMMON_SOURCES})
set_property(TARGET popular_common PROPERTY POSITION_INDEPENDENT_CODE ON)
