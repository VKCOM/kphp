set(VKEXT_DIR ${BASE_DIR}/vkext)

function(install_vkext PHP_VERSION)
    execute_process(COMMAND php-config${PHP_VERSION} --extension-dir
                    OUTPUT_VARIABLE VKEXT_LIB_DESTINATION
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(COMPONENT_NAME "VKEXT${PHP_VERSION}")

    install(FILES ${OBJS_DIR}/vkext/modules${PHP_VERSION}/vkext.so
            COMPONENT ${COMPONENT_NAME}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/${VKEXT_LIB_DESTINATION})

    install(FILES ${VKEXT_DIR}/vkext.ini
            COMPONENT ${COMPONENT_NAME}
            DESTINATION etc/php/${PHP_VERSION}/mods-available)

    install_symlink("etc/php/${PHP_VERSION}/mods-available/vkext.ini"
                    "etc/php/${PHP_VERSION}/apache2/conf.d/20-vkext.ini"
                    ${COMPONENT_NAME})
    install_symlink("etc/php/${PHP_VERSION}/mods-available/vkext.ini"
                    "etc/php/${PHP_VERSION}/cli/conf.d/20-vkext.ini"
                    ${COMPONENT_NAME})

    set(PHP php${PHP_VERSION})
    set(CPACK_DEBIAN_${COMPONENT_NAME}_PACKAGE_DEPENDS "${PHP}-cli, vk-flex-data, ${PHP}-mbstring, ${PHP}-bcmath, ${PHP}-curl" PARENT_SCOPE)
    set(CPACK_DEBIAN_${COMPONENT_NAME}_DESCRIPTION "${PHP} extension for supporting kphp-specific functions in PHP" PARENT_SCOPE)
    set(CPACK_DEBIAN_${COMPONENT_NAME}_PACKAGE_NAME "${PHP}-vkext" PARENT_SCOPE)

    set(CONFFILES_FILE "${CMAKE_CURRENT_BINARY_DIR}/conffiles${PHP_VERSION}/conffiles")
    file(WRITE "${CONFFILES_FILE}" "${CMAKE_INSTALL_PREFIX}/etc/php/${PHP_VERSION}/mods-available/vkext.ini\n")
    set(CPACK_DEBIAN_${COMPONENT_NAME}_PACKAGE_CONTROL_EXTRA "${CONFFILES_FILE}" PARENT_SCOPE)
endfunction()

prepend(VKEXT_COMMON_SOURCES ${COMMON_DIR}/
        crc32.cpp
        crc32_x86_64.cpp
        string-processing.cpp
        unicode/utf8-utils.cpp
        cpuid.cpp
        version-string.cpp)

prepend(VKEXT_SOURCES ${VKEXT_DIR}/
        vkext.cpp
        vkext-iconv.cpp
        vkext-flex.cpp
        vkext-json.cpp
        vkext-rpc.cpp
        vkext-tl-parse.cpp
        vkext-tl-memcache.cpp
        vkext-schema-memcache.cpp
        vkext-errors.cpp
        vkext-stats.cpp
        vkext-sp.cpp)

foreach(PHP_VERSION IN ITEMS "" "7.2" "7.4")
    find_program(PHP_CONFIG_EXEC${PHP_VERSION} php-config${PHP_VERSION})
    set(PHP_CONFIG_EXEC ${PHP_CONFIG_EXEC${PHP_VERSION}})
    if(PHP_CONFIG_EXEC)
        message(STATUS "${PHP_CONFIG_EXEC} found")
        execute_process(COMMAND ${PHP_CONFIG_EXEC} --include-dir
                        OUTPUT_VARIABLE PHP_SOURCE
                        OUTPUT_STRIP_TRAILING_WHITESPACE)

        set(VKEXT_TARGET vkext${PHP_VERSION})
        vk_add_library(${VKEXT_TARGET} SHARED ${VKEXT_SOURCES} ${VKEXT_COMMON_SOURCES})
        if(APPLE)
            target_link_options(${VKEXT_TARGET} PRIVATE -undefined dynamic_lookup)
        endif()
        target_compile_definitions(${VKEXT_TARGET} PRIVATE -DVKEXT -DPHP_ATOM_INC)
        target_compile_options(${VKEXT_TARGET} PRIVATE -mfpmath=sse -Wno-unused-parameter -Wno-float-conversion -Wno-ignored-qualifiers)
        target_include_directories(${VKEXT_TARGET} PRIVATE ${PHP_SOURCE} ${PHP_SOURCE}/main ${PHP_SOURCE}/Zend ${PHP_SOURCE}/TSRM ${BASE_DIR})
        target_link_libraries(${VKEXT_TARGET} PRIVATE vk::flex_data_shared)
        set_target_properties(${VKEXT_TARGET} PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY ${OBJS_DIR}/vkext/modules${PHP_VERSION}/
                LIBRARY_OUTPUT_NAME vkext
                PREFIX "")

        set_source_files_properties(${VKEXT_DIR}/vkext-rpc.cpp PROPERTIES COMPILE_FLAGS -Wno-format-security)
        set_source_files_properties(${VKEXT_DIR}/vkext.cpp PROPERTIES COMPILE_FLAGS -Wno-unused-result)

        if(NO_SANITIZE_IS_FOUND)
            target_compile_options(${VKEXT_TARGET} PRIVATE -fno-sanitize=all)
            target_link_options(${VKEXT_TARGET} PRIVATE -fno-sanitize=all)
        endif()

        if(PHP_VERSION)
            install_vkext(${PHP_VERSION})
        endif()
    endif()
endforeach()
