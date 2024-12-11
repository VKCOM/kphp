include_guard(GLOBAL)

set(FLEX_DIR ${BASE_DIR}/flex)
set(AUTO_FLEX_DIR ${AUTO_DIR}/flex)
set(FLEX_DATA_SRC ${AUTO_FLEX_DIR}/vk-flex-data.cpp)
set(FLEX_SOURCES ${FLEX_DATA_SRC} ${FLEX_DIR}/flex.cpp)

file(MAKE_DIRECTORY ${AUTO_FLEX_DIR})
add_custom_command(
        OUTPUT ${FLEX_DATA_SRC}
        COMMAND ${PHP_BIN} ${FLEX_DIR}/vk-flex-data-gen.php > ${FLEX_DATA_SRC}
        DEPENDS ${FLEX_DIR}/vk-flex-data-gen.php
                ${FLEX_DIR}/lib/vkext-flex-generate.lib.php
                ${FLEX_DIR}/lib/configs/flex-config.php
        COMMENT "vk-flex-data generation")

if(COMPILER_CLANG)
    set_source_files_properties(${FLEX_DATA_SRC} PROPERTIES COMPILE_FLAGS -Wno-invalid-source-encoding)
endif()

vk_add_library(flex_data_shared SHARED ${FLEX_SOURCES})
vk_add_library(flex_data_static STATIC ${FLEX_SOURCES})
check_cxx_compiler_flag(-fno-sanitize=all NO_SANITIZE_IS_FOUND)
if(NO_SANITIZE_IS_FOUND)
    target_compile_options(flex_data_shared PRIVATE -fno-sanitize=all)
    target_link_options(flex_data_shared PRIVATE -fno-sanitize=all)
endif()
# to prevent double generation of ${FLEX_DATA_SRC}
add_dependencies(flex_data_shared flex_data_static)

set_target_properties(flex_data_shared flex_data_static
                      PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OBJS_DIR}/flex
                                 ARCHIVE_OUTPUT_DIRECTORY ${OBJS_DIR}/flex
                                 OUTPUT_NAME vk-flex-data)

install(TARGETS flex_data_shared flex_data_static
        COMPONENT FLEX
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(CPACK_DEBIAN_FLEX_PACKAGE_BREAKS "engine-kphp-runtime (<< 20190917), php5-vkext (<< 20190917), php7-vkext (<< 20190917)")
set(CPACK_DEBIAN_FLEX_PACKAGE_NAME "vk-flex-data")
set(CPACK_DEBIAN_FLEX_DESCRIPTION "flex for declension")
set(CPACK_DEBIAN_FLEX_PACKAGE_CONTROL_EXTRA "${BASE_DIR}/cmake/triggers")
set(CPACK_DEBIAN_FLEX_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
