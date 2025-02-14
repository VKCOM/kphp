update_git_submodule(${THIRD_PARTY_DIR}/zlib "--recursive")

set(ZLIB_SOURCE_DIR  ${THIRD_PARTY_DIR}/zlib)

# The configuration has been based on:
# https://sources.debian.org/src/zlib/1%3A1.3.dfsg%2Breally1.3.1-1/debian/rules/#L20
set(ZLIB_COMMON_COMPILE_FLAGS "$ENV{CFLAGS} -g0 -Wall -O3 -D_REENTRANT")

if(APPLE)
    detect_xcode_sdk_path(CMAKE_OSX_SYSROOT)
    set(ZLIB_COMMON_COMPILE_FLAGS "${ZLIB_COMMON_COMPILE_FLAGS} --sysroot ${CMAKE_OSX_SYSROOT}")
endif()

include(${THIRD_PARTY_DIR}/zlib-cmake/zlib-pic.cmake)
include(${THIRD_PARTY_DIR}/zlib-cmake/zlib-no-pic.cmake)

set(ZLIB_FOUND ON)
