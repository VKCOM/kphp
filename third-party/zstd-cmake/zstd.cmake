update_git_submodule(${THIRD_PARTY_DIR}/zstd "--remote")

set(ZSTD_SOURCE_DIR     ${THIRD_PARTY_DIR}/zstd)
set(ZSTD_BUILD_DIR      ${CMAKE_BINARY_DIR}/third-party/zstd/build)
set(ZSTD_INSTALL_DIR    ${CMAKE_BINARY_DIR}/third-party/zstd/install)
set(ZSTD_BINARY_DIR     ${ZSTD_BUILD_DIR}/lib)
set(ZSTD_LIBRARIES      ${ZSTD_INSTALL_DIR}/lib/libzstd.a)
set(ZSTD_INCLUDE_DIRS   ${ZSTD_INSTALL_DIR}/include)
set(ZSTD_PATCH_DIR      ${ZSTD_BUILD_DIR}/debian/patches/)
set(ZSTD_PATCH_SERIES   ${ZSTD_BUILD_DIR}/debian/patches/series)
# Ensure the build, installation and "include" directories exists
file(MAKE_DIRECTORY ${ZSTD_BUILD_DIR})
file(MAKE_DIRECTORY ${ZSTD_INSTALL_DIR})
file(MAKE_DIRECTORY ${ZSTD_INCLUDE_DIRS})

# The configuration has been based on:
# https://sources.debian.org/src/libzstd/1.4.8%2Bdfsg-2.1/debian/rules/
set(ZSTD_COMPILE_FLAGS "$ENV{CFLAGS} -g0 -fno-pic -Wno-unused-but-set-variable")
if(NOT APPLE)
    set(ZSTD_COMPILE_FLAGS "${ZSTD_COMPILE_FLAGS} -static")
endif()

set(ZSTD_MAKE_ARGS
        CC=${CMAKE_C_COMPILER}
        CFLAGS=${OPENSSL_COMPILE_FLAGS}
)

set(ZSTD_MAKE_INSTALL_ARGS
        PREFIX=${ZSTD_INSTALL_DIR}
        INCLUDEDIR=${ZSTD_INSTALL_DIR}/include/zstd
)

ExternalProject_Add(
        zstd
        PREFIX ${ZSTD_BUILD_DIR}
        SOURCE_DIR ${ZSTD_SOURCE_DIR}
        INSTALL_DIR ${ZSTD_INSTALL_DIR}
        BINARY_DIR ${ZSTD_BINARY_DIR}
        BUILD_BYPRODUCTS ${ZSTD_INSTALL_DIR}/lib/libzstd.a
        PATCH_COMMAND
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${ZSTD_SOURCE_DIR} ${ZSTD_BUILD_DIR}
            COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${ZSTD_BUILD_DIR} -DPATCH_SERIES=${ZSTD_PATCH_SERIES} -DPATCH_DIR=${ZSTD_PATCH_DIR} -P ../../cmake/apply_patches.cmake
        CONFIGURE_COMMAND
            COMMAND # Nothing to configure
        BUILD_COMMAND
            COMMAND ${CMAKE_COMMAND} -E env ${ZSTD_MAKE_ARGS} make libzstd.a -j
        INSTALL_COMMAND
            COMMAND ${CMAKE_COMMAND} -E env ${ZSTD_MAKE_INSTALL_ARGS} make install-static install-includes
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${ZSTD_INCLUDE_DIRS} ${INCLUDE_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${ZSTD_LIBRARIES} ${LIB_DIR}
        BUILD_IN_SOURCE 0
)

add_library(ZSTD::zstd STATIC IMPORTED)
set_target_properties(ZSTD::zstd PROPERTIES
        IMPORTED_LOCATION ${ZSTD_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${ZSTD_INCLUDE_DIRS}
)

# Set variables indicating that zstd has been installed
set(ZSTD_FOUND ON)

cmake_print_variables(ZSTD_LIBRARIES)
