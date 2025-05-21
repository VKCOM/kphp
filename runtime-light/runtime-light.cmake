include(${THIRD_PARTY_DIR}/timelib-cmake/timelib.cmake)
include(${THIRD_PARTY_DIR}/pcre2-cmake/pcre2.cmake)

# =================================================================================================

set(RUNTIME_LIGHT_COMPILE_FLAGS -O0 -g -stdlib=libc++)

set(RUNTIME_LIGHT_PLATFORM_SPECIFIC_LINK_FLAGS)
if(APPLE)
  set(RUNTIME_LIGHT_PLATFORM_SPECIFIC_LINK_FLAGS -undefined dynamic_lookup)
else()
  set(RUNTIME_LIGHT_PLATFORM_SPECIFIC_LINK_FLAGS --allow-shlib-undefined)
endif()

set(RUNTIME_LIGHT_LINK_FLAGS -stdlib=libc++ -static-libstdc++ -static-libgcc ${RUNTIME_LIGHT_PLATFORM_SPECIFIC_LINK_FLAGS})

# =================================================================================================

include(${RUNTIME_LIGHT_DIR}/allocator/allocator.cmake)
include(${RUNTIME_LIGHT_DIR}/core/core.cmake)
include(${RUNTIME_LIGHT_DIR}/coroutine/coroutine.cmake)
include(${RUNTIME_LIGHT_DIR}/scheduler/scheduler.cmake)
include(${RUNTIME_LIGHT_DIR}/server/server.cmake)
include(${RUNTIME_LIGHT_DIR}/stdlib/stdlib.cmake)
include(${RUNTIME_LIGHT_DIR}/streams/streams.cmake)
include(${RUNTIME_LIGHT_DIR}/tl/tl.cmake)
include(${RUNTIME_LIGHT_DIR}/utils/utils.cmake)
include(${RUNTIME_LIGHT_DIR}/state/state.cmake)
include(${RUNTIME_LIGHT_DIR}/memory-resource-impl/memory-resource-impl.cmake)

set(RUNTIME_LIGHT_SRC
    ${RUNTIME_LIGHT_CORE_SRC}
    ${RUNTIME_LIGHT_COROUTINE_SRC}
    ${RUNTIME_LIGHT_STDLIB_SRC}
    ${RUNTIME_LIGHT_SCHEDULER_SRC}
    ${RUNTIME_LIGHT_SERVER_SRC}
    ${RUNTIME_LIGHT_ALLOCATOR_SRC}
    ${RUNTIME_LIGHT_COROUTINE_SRC}
    ${RUNTIME_LIGHT_STATE_SRC}
    ${RUNTIME_LIGHT_STREAMS_SRC}
    ${RUNTIME_LIGHT_TL_SRC}
    ${RUNTIME_LIGHT_UTILS_SRC}
    ${RUNTIME_LIGHT_MEMORY_RESOURCE_IMPL_SRC}
    runtime-light.cpp)

set(RUNTIME_SOURCES_FOR_COMP "${RUNTIME_LIGHT_SRC}")
configure_file(${BASE_DIR}/compiler/runtime_sources.h.in ${AUTO_DIR}/compiler/runtime_sources.h)

prepend(RUNTIME_LIGHT_SRC ${RUNTIME_LIGHT_DIR}/ "${RUNTIME_LIGHT_SRC}")

# =================================================================================================

# include alloc wrapper
include(${RUNTIME_LIGHT_DIR}/allocator-wrapper/allocator-wrapper.cmake)

vk_add_library_pic(runtime-light-pic OBJECT ${RUNTIME_LIGHT_SRC})
target_compile_options(runtime-light-pic PUBLIC ${RUNTIME_LIGHT_COMPILE_FLAGS})
target_link_libraries(runtime-light-pic PUBLIC vk::pic::libc-alloc-wrapper) # it's mandatory to have alloc-wrapper first in the list of link libraries since we
                                                                            # want to use its symbols in all other libraries
target_link_libraries(runtime-light-pic PUBLIC KPHP_TIMELIB::pic::timelib PCRE2::pic::pcre2 ZLIB::pic::zlib) # third parties

set(RUNTIME_LIGHT_LINK_LIBS "${KPHP_TIMELIB_PIC_LIBRARIES} ${PCRE2_PIC_LIBRARIES} ${ZLIB_PIC_LIBRARIES}")

# =================================================================================================

vk_add_library_pic(kphp-light-runtime-pic STATIC)
target_compile_options(kphp-light-runtime-pic PUBLIC ${RUNTIME_LIGHT_COMPILE_FLAGS})
target_link_options(kphp-light-runtime-pic PUBLIC ${RUNTIME_LIGHT_LINK_FLAGS})
target_link_libraries(kphp-light-runtime-pic PUBLIC vk::pic::light-common vk::pic::unicode vk::pic::runtime-light vk::pic::runtime-common)

set_target_properties(kphp-light-runtime-pic PROPERTIES LIBRARY_OUTPUT_NAME libkphp-light-runtime.a)

combine_static_runtime_library(kphp-light-runtime-pic k2kphp-rt)

# =================================================================================================

file(
  GLOB_RECURSE KPHP_RUNTIME_ALL_HEADERS
  RELATIVE ${BASE_DIR}
  CONFIGURE_DEPENDS "${RUNTIME_LIGHT_DIR}/*.h")
file(
  GLOB_RECURSE KPHP_RUNTIME_COMMON_ALL_HEADERS
  RELATIVE ${BASE_DIR}
  CONFIGURE_DEPENDS "${RUNTIME_COMMON_DIR}/*.h")
list(APPEND KPHP_RUNTIME_ALL_HEADERS ${KPHP_RUNTIME_COMMON_ALL_HEADERS})
list(TRANSFORM KPHP_RUNTIME_ALL_HEADERS REPLACE "^(.+)$" [[#include "\1"]])
list(JOIN KPHP_RUNTIME_ALL_HEADERS "\n" MERGED_RUNTIME_HEADERS)
file(
  WRITE ${AUTO_DIR}/runtime/runtime-headers.h
  "\
#ifndef MERGED_RUNTIME_LIGHT_HEADERS_H
#define MERGED_RUNTIME_LIGHT_HEADERS_H

${MERGED_RUNTIME_HEADERS}

#endif
")

file(
  WRITE ${CMAKE_CURRENT_BINARY_DIR}/php_lib_version.cpp
  [[
#include "auto/runtime/runtime-headers.h"
]])

# =================================================================================================

add_library(php_lib_version_j OBJECT ${CMAKE_CURRENT_BINARY_DIR}/php_lib_version.cpp)
target_compile_options(php_lib_version_j PRIVATE -stdlib=libc++ -iquote ${AUTO_DIR} -E)
target_link_options(php_lib_version_j PUBLIC -stdlib=libc++ -static-libstdc++)
add_dependencies(php_lib_version_j kphp-light-runtime-pic)

add_custom_command(
  OUTPUT ${OBJS_DIR}/php_lib_version.sha256
  COMMAND tail -n +3 $<TARGET_OBJECTS:php_lib_version_j> | sha256sum | awk
          '{print $$1}' > ${OBJS_DIR}/php_lib_version.sha256
  DEPENDS php_lib_version_j $<TARGET_OBJECTS:php_lib_version_j>
  COMMENT "php_lib_version.sha256 generation")

add_custom_target(php_lib_version_sha_256 DEPENDS ${OBJS_DIR}/php_lib_version.sha256)

# =================================================================================================

get_target_property(RUNTIME_LIGHT_COMPILE_FLAGS kphp-light-runtime-pic COMPILE_OPTIONS)
list(JOIN RUNTIME_LIGHT_COMPILE_FLAGS "\;" RUNTIME_LIGHT_COMPILE_FLAGS)
string(REPLACE "\"" "\\\"" RUNTIME_LIGHT_COMPILE_FLAGS ${RUNTIME_LIGHT_COMPILE_FLAGS})
configure_file(${BASE_DIR}/compiler/runtime_compile_flags.h.in ${AUTO_DIR}/compiler/runtime_compile_flags.h)

list(JOIN RUNTIME_LIGHT_LINK_LIBS "\;" RUNTIME_LIGHT_LINK_LIBS)
string(REPLACE "\"" "\\\"" RUNTIME_LIGHT_LINK_LIBS ${RUNTIME_LIGHT_LINK_LIBS})
configure_file(${BASE_DIR}/compiler/runtime_link_libs.h.in ${AUTO_DIR}/compiler/runtime_link_libs.h)

get_target_property(RUNTIME_LIGHT_LINK_FLAGS kphp-light-runtime-pic LINK_OPTIONS)
list(JOIN RUNTIME_LIGHT_LINK_FLAGS "\;" RUNTIME_LIGHT_LINK_FLAGS)
string(REPLACE "\"" "\\\"" RUNTIME_LIGHT_LINK_FLAGS ${RUNTIME_LIGHT_LINK_FLAGS})
configure_file(${BASE_DIR}/compiler/runtime_link_flags.h.in ${AUTO_DIR}/compiler/runtime_link_flags.h)
