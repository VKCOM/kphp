set(ZLIB_PIC_BUILD_DIR      ${CMAKE_BINARY_DIR}/third-party/zlib-pic/build)
set(ZLIB_PIC_INSTALL_DIR    ${CMAKE_BINARY_DIR}/third-party/zlib-pic/install)
set(ZLIB_PIC_INCLUDE_DIRS   ${ZLIB_PIC_INSTALL_DIR}/include)
set(ZLIB_PIC_LIBRARIES      ${ZLIB_PIC_INSTALL_DIR}/lib/libz-pic.a)
# Ensure the build, installation and "include" directories exists
file(MAKE_DIRECTORY ${ZLIB_PIC_BUILD_DIR})
file(MAKE_DIRECTORY ${ZLIB_PIC_INSTALL_DIR})
file(MAKE_DIRECTORY ${ZLIB_PIC_INCLUDE_DIRS})

# For further optional differences
if(COMPILE_RUNTIME_LIGHT)
    set(ZLIB_PIC_COMPILE_FLAGS "${ZLIB_COMMON_COMPILE_FLAGS} -fPIC")
else()
    set(ZLIB_PIC_COMPILE_FLAGS "${ZLIB_COMMON_COMPILE_FLAGS} -fPIC")
endif()

ExternalProject_Add(
        zlib-pic
        PREFIX ${ZLIB_PIC_BUILD_DIR}
        SOURCE_DIR ${ZLIB_SOURCE_DIR}
        INSTALL_DIR ${ZLIB_PIC_INSTALL_DIR}
        BINARY_DIR ${ZLIB_PIC_BUILD_DIR}
        BUILD_BYPRODUCTS ${ZLIB_PIC_INSTALL_DIR}/lib/libz-pic.a
        CONFIGURE_COMMAND
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${ZLIB_SOURCE_DIR} ${ZLIB_PIC_BUILD_DIR}
            COMMAND ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} CFLAGS=${ZLIB_PIC_COMPILE_FLAGS} ./configure --prefix=${ZLIB_PIC_INSTALL_DIR} --includedir=${ZLIB_PIC_INCLUDE_DIRS}/zlib --static
        BUILD_COMMAND
            COMMAND make libz.a -j
        INSTALL_COMMAND
            COMMAND make install
            COMMAND ${CMAKE_COMMAND} -E copy ${ZLIB_PIC_INSTALL_DIR}/lib/libz.a ${ZLIB_PIC_INSTALL_DIR}/lib/libz-pic.a
            COMMAND ${CMAKE_COMMAND} -E copy ${ZLIB_PIC_INSTALL_DIR}/lib/libz-pic.a ${LIB_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${ZLIB_PIC_INCLUDE_DIRS} ${INCLUDE_DIR}
        BUILD_IN_SOURCE 0
)

add_library(ZLIB::ZLIB_PIC STATIC IMPORTED)
set_target_properties(ZLIB::ZLIB_PIC PROPERTIES
        IMPORTED_LOCATION ${ZLIB_PIC_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${ZLIB_PIC_INCLUDE_DIRS}
)

# Ensure that the zlib are built before they are used
add_dependencies(ZLIB::ZLIB_PIC zlib-pic)

# Set variables indicating that zlib has been installed
set(ZLIB_PIC_ROOT ${ZLIB_PIC_INSTALL_DIR})

cmake_print_variables(ZLIB_PIC_LIBRARIES)
