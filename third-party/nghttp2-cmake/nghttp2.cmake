update_git_submodule(${THIRD_PARTY_DIR}/nghttp2 "--remote")

set(NGHTTP2_SOURCE_DIR      ${THIRD_PARTY_DIR}/nghttp2)
set(NGHTTP2_BUILD_DIR       ${CMAKE_BINARY_DIR}/third-party/nghttp2/build)
set(NGHTTP2_INSTALL_DIR     ${CMAKE_BINARY_DIR}/third-party/nghttp2/install)
set(NGHTTP2_LIBRARIES       ${NGHTTP2_INSTALL_DIR}/lib/libnghttp2.a)
set(NGHTTP2_INCLUDE_DIRS    ${NGHTTP2_INSTALL_DIR}/include)
set(NGHTTP2_PATCH_DIR       ${NGHTTP2_BUILD_DIR}/debian/patches/)
set(NGHTTP2_PATCH_SERIES    ${NGHTTP2_BUILD_DIR}/debian/patches/series)
# Ensure the build, installation and "include" directories exists
file(MAKE_DIRECTORY ${NGHTTP2_BUILD_DIR})
file(MAKE_DIRECTORY ${NGHTTP2_INSTALL_DIR})
file(MAKE_DIRECTORY ${NGHTTP2_INCLUDE_DIRS})

set(NGHTTP2_COMPILE_FLAGS "$ENV{CFLAGS} -g0 -fno-pic")
if(NOT APPLE)
    set(NGHTTP2_COMPILE_FLAGS "${NGHTTP2_COMPILE_FLAGS} -static")
endif()

# The configuration has been based on:
# https://salsa.debian.org/debian/nghttp2/-/blob/buster/debian/rules#L8
set(NGHTTP2_CMAKE_ARGS
        -DCMAKE_C_FLAGS=${NGHTTP2_COMPILE_FLAGS}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_FLAGS=${NGHTTP2_COMPILE_FLAGS}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_POSITION_INDEPENDENT_CODE=OFF
        -DENABLE_WERROR=ON
        -DENABLE_THREADS=OFF
        -DENABLE_APP=OFF
        -DENABLE_HPACK_TOOLS=OFF
        -DENABLE_ASIO_LIB=OFF
        -DENABLE_EXAMPLES=OFF
        -DENABLE_PYTHON_BINDINGS=OFF
        -DENABLE_FAILMALLOC=OFF
        -DENABLE_LIB_ONLY=ON
        -DENABLE_STATIC_LIB=ON
        -DENABLE_SHARED_LIB=OFF
        -DWITH_LIBXML2=OFF
        -DWITH_JEMALLOC=OFF
        -DWITH_SPDYLAY=OFF
        -DWITH_MRUBY=OFF
        -DWITH_NEVERBLEED=OFF
        -DOPENSSL_FOUND=ON
        -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}
        -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES}
        -DOPENSSL_INCLUDE_DIR=${OPENSSL_INCLUDE_DIR}
        -DZLIB_FOUND=ON
        -DZLIB_ROOT=${ZLIB_NO_PIC_ROOT}
        -DZLIB_LIBRARIES=${ZLIB_NO_PIC_LIBRARIES}
        -DZLIB_INCLUDE_DIR=${ZLIB_NO_PIC_INCLUDE_DIRS}/zlib
)

ExternalProject_Add(
        nghttp2
        DEPENDS openssl zlib-no-pic
        PREFIX ${NGHTTP2_BUILD_DIR}
        SOURCE_DIR ${NGHTTP2_SOURCE_DIR}
        INSTALL_DIR ${NGHTTP2_INSTALL_DIR}
        BINARY_DIR ${NGHTTP2_BUILD_DIR}
        BUILD_BYPRODUCTS ${NGHTTP2_INSTALL_DIR}/lib/libnghttp2.a
        PATCH_COMMAND
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${NGHTTP2_SOURCE_DIR} ${NGHTTP2_BUILD_DIR}
            COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${NGHTTP2_BUILD_DIR} -DPATCH_SERIES=${NGHTTP2_PATCH_SERIES} -DPATCH_DIR=${NGHTTP2_PATCH_DIR} -P ../../cmake/apply_patches.cmake
        CONFIGURE_COMMAND
            COMMAND ${CMAKE_COMMAND} ${NGHTTP2_CMAKE_ARGS} -S ${NGHTTP2_SOURCE_DIR} -B ${NGHTTP2_BUILD_DIR} -Wno-dev
        BUILD_COMMAND
            COMMAND ${CMAKE_COMMAND} --build ${NGHTTP2_BUILD_DIR} --config $<CONFIG> -j
        INSTALL_COMMAND
            COMMAND ${CMAKE_COMMAND} --install ${NGHTTP2_BUILD_DIR} --prefix ${NGHTTP2_INSTALL_DIR} --config $<CONFIG>
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${NGHTTP2_INCLUDE_DIRS} ${INCLUDE_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${NGHTTP2_LIBRARIES} ${LIB_DIR}
        BUILD_IN_SOURCE 0
)

add_library(NGHTTP2::nghttp2 STATIC IMPORTED)
set_target_properties(NGHTTP2::nghttp2 PROPERTIES
        IMPORTED_LOCATION ${NGHTTP2_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${NGHTTP2_INCLUDE_DIRS}
)

add_dependencies(NGHTTP2::nghttp2 nghttp2)

########################################
add_library(NGHTTP2::pic::nghttp2 STATIC IMPORTED)
set_target_properties(NGHTTP2::pic::nghttp2 PROPERTIES
        IMPORTED_LOCATION ${NGHTTP2_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${NGHTTP2_INCLUDE_DIRS}
)

add_dependencies(NGHTTP2::pic::nghttp2 nghttp2)

add_library(NGHTTP2::no-pic::nghttp2 STATIC IMPORTED)
set_target_properties(NGHTTP2::no-pic::nghttp2 PROPERTIES
        IMPORTED_LOCATION ${NGHTTP2_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${NGHTTP2_INCLUDE_DIRS}
)

add_dependencies(NGHTTP2::no-pic::nghttp2 nghttp2)
###############################################


# Set variables indicating that NGHTTP2 has been installed
set(NGHTTP2_FOUND ON)
set(NGHTTP2_ROOT ${NGHTTP2_INSTALL_DIR})

cmake_print_variables(NGHTTP2_LIBRARIES)
