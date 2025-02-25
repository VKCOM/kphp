update_git_submodule(${THIRD_PARTY_DIR}/zlib "--recursive")

function(build_zlib PIC_MODE)
    set(PROJECT_GENERIC_NAME zlib)
    set(TARGET_NAMESPACE ZLIB)
    set(TARGET_GENERIC_NAME ZLIB)

    set(PROJECT_NAME)
    set(TARGET_NAME)
    set(EXTRA_COMPILE_FLAGS)
    set(SUFFIX)

    if(PIC_MODE)
        set(SUFFIX "-pic")
        set(EXTRA_COMPILE_FLAGS "-fPIC")
        set(TARGET_NAME "${TARGET_GENERIC_NAME}_PIC")
    else()
        set(SUFFIX "-no-pic")
        set(EXTRA_COMPILE_FLAGS "-fno-pic")
        if(NOT APPLE)
            set(EXTRA_COMPILE_FLAGS "${EXTRA_COMPILE_FLAGS} -static")
        endif()
        set(TARGET_NAME "${TARGET_GENERIC_NAME}_NO_PIC")
    endif()

    if(APPLE)
        set(EXTRA_COMPILE_FLAGS "${EXTRA_COMPILE_FLAGS} --sysroot ${CMAKE_OSX_SYSROOT}")
    endif()

    set(PROJECT_NAME "${PROJECT_GENERIC_NAME}${SUFFIX}")

    set(SOURCE_DIR      ${THIRD_PARTY_DIR}/zlib)
    set(BUILD_DIR       ${CMAKE_BINARY_DIR}/third-party/${PROJECT_NAME}/build)
    set(INSTALL_DIR     ${CMAKE_BINARY_DIR}/third-party/${PROJECT_NAME}/install)
    set(INCLUDE_DIRS    ${INSTALL_DIR}/include)
    set(LIBRARIES       ${INSTALL_DIR}/lib/libz${SUFFIX}.a)
    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${BUILD_DIR})
    file(MAKE_DIRECTORY ${INSTALL_DIR})
    file(MAKE_DIRECTORY ${INCLUDE_DIRS})

    set(COMPILE_FLAGS "$ENV{CFLAGS} -g0 -Wall -O3 -D_REENTRANT ${EXTRA_COMPILE_FLAGS}")

    message(STATUS "Zlib Summary:

        PIC mode:       ${PIC_MODE}
        Source dir:     ${SOURCE_DIR}
        Build dir:      ${BUILD_DIR}
        Install dir:    ${INSTALL_DIR}
        Include dirs:   ${INCLUDE_DIRS}
        Libraries:      ${LIBRARIES}
        Compiler:
          C compiler:   ${CMAKE_C_COMPILER}
          CFLAGS:       ${COMPILE_FLAGS}
    ")

    ExternalProject_Add(
            ${PROJECT_NAME}
            PREFIX ${BUILD_DIR}
            SOURCE_DIR ${SOURCE_DIR}
            INSTALL_DIR ${INSTALL_DIR}
            BINARY_DIR ${BUILD_DIR}
            BUILD_BYPRODUCTS ${LIBRARIES}
            CONFIGURE_COMMAND
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${SOURCE_DIR} ${BUILD_DIR}
                COMMAND ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} CFLAGS=${COMPILE_FLAGS} ./configure --prefix=${INSTALL_DIR} --includedir=${INCLUDE_DIRS}/zlib --static
            BUILD_COMMAND
                COMMAND make libz.a -j
            INSTALL_COMMAND
                COMMAND make install
                COMMAND ${CMAKE_COMMAND} -E copy ${INSTALL_DIR}/lib/libz.a ${LIBRARIES}
                COMMAND ${CMAKE_COMMAND} -E copy ${LIBRARIES} ${LIB_DIR}
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${INCLUDE_DIRS} ${INCLUDE_DIR}
            BUILD_IN_SOURCE 0
    )

    add_library(${TARGET_NAMESPACE}::${TARGET_NAME} STATIC IMPORTED)
    set_target_properties(${TARGET_NAMESPACE}::${TARGET_NAME} PROPERTIES
            IMPORTED_LOCATION ${LIBRARIES}
            INTERFACE_INCLUDE_DIRECTORIES ${INCLUDE_DIRS}
    )

    # Ensure that the zlib are built before they are used
    add_dependencies(${TARGET_NAMESPACE}::${TARGET_NAME} ${PROJECT_NAME})

    ######################################
    if(PIC_MODE)
        add_library(${TARGET_NAMESPACE}::pic::zlib STATIC IMPORTED)
        set_target_properties(${TARGET_NAMESPACE}::pic::zlib PROPERTIES
                IMPORTED_LOCATION ${LIBRARIES}
                INTERFACE_INCLUDE_DIRECTORIES ${INCLUDE_DIRS}
        )
        add_dependencies(${TARGET_NAMESPACE}::pic::zlib ${PROJECT_NAME})
    else()
        add_library(${TARGET_NAMESPACE}::no-pic::zlib STATIC IMPORTED)
        set_target_properties(${TARGET_NAMESPACE}::no-pic::zlib PROPERTIES
                IMPORTED_LOCATION ${LIBRARIES}
                INTERFACE_INCLUDE_DIRECTORIES ${INCLUDE_DIRS}
        )
        add_dependencies(${TARGET_NAMESPACE}::no-pic::zlib ${PROJECT_NAME})
    endif()
    ######################################

    # Set variables indicating that zlib has been installed
    set(${TARGET_NAME}_ROOT ${INSTALL_DIR} PARENT_SCOPE)
    set(${TARGET_NAME}_INCLUDE_DIRS ${INCLUDE_DIRS} PARENT_SCOPE)
    set(${TARGET_NAME}_LIBRARIES ${LIBRARIES} PARENT_SCOPE)
    set(ZLIB_FOUND ON PARENT_SCOPE)
endfunction()

# PIC is OFF
build_zlib(OFF)
# PIC is ON
build_zlib(ON)
