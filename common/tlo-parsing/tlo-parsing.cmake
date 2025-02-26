include_guard(GLOBAL)

prepend(TLO_PARSING_OBJECTS ${COMMON_DIR}/tlo-parsing/
        flat-optimization.cpp
        remove-odd-types.cpp
        replace-anonymous-args.cpp
        tl-dependency-graph.cpp
        tl-objects.cpp
        tl-scheme-final-check.cpp
        tlo-parser.cpp)

vk_add_library_pic(tlo_parsing_src-pic OBJECT ${TLO_PARSING_OBJECTS})
vk_add_library_no_pic(tlo_parsing_src-no-pic OBJECT ${TLO_PARSING_OBJECTS})

prepend(TLO_PARSING_PUBLIC_HEADERS ${COMMON_DIR}/tlo-parsing/
        tl-objects.h
        tl-dependency-graph.h
        tlo-parsing.h)

vk_add_library_no_pic(tlo_parsing_static-no-pic STATIC $<TARGET_OBJECTS:tlo_parsing_src-no-pic>)
set_target_properties(tlo_parsing_static-no-pic PROPERTIES
        OUTPUT_NAME tlo_parsing
        PUBLIC_HEADER "${TLO_PARSING_PUBLIC_HEADERS}")

install(TARGETS tlo_parsing_static-no-pic
        COMPONENT TLO_PARSING_DEV
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tlo-parsing)

vk_add_library_pic(tlo_parsing_shared-pic SHARED $<TARGET_OBJECTS:tlo_parsing_src-pic>)
set_target_properties(tlo_parsing_shared-pic PROPERTIES OUTPUT_NAME tlo_parsing)

if(NOT APPLE)
    target_link_options(tlo_parsing_shared-pic PRIVATE -Wl,--version-script=${COMMON_DIR}/tlo-parsing/tlo-parsing-lib-exported-symbols.map)
endif()

install(TARGETS tlo_parsing_shared-pic
        COMPONENT TLO_PARSING
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(CPACK_DEBIAN_TLO_PARSING_PACKAGE_NAME "libtlo-parsing")
set(CPACK_DEBIAN_TLO_PARSING_DESCRIPTION "Library files for the tlo parsing")
set(CPACK_DEBIAN_TLO_PARSING_PACKAGE_CONTROL_EXTRA "${BASE_DIR}/cmake/triggers")
set(CPACK_DEBIAN_TLO_PARSING_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)

set(CPACK_DEBIAN_TLO_PARSING_DEV_PACKAGE_NAME "libtlo-parsing-dev")
set(CPACK_DEBIAN_TLO_PARSING_DEV_DESCRIPTION "Development files and headers for the tlo parsing")
set(CPACK_DEBIAN_TLO_PARSING_DEV_PACKAGE_DEPENDS "libtlo-parsing")
set(CPACK_DEBIAN_TLO_PARSING_DEV_PACKAGE_CONTROL_EXTRA "${BASE_DIR}/cmake/triggers")
set(CPACK_DEBIAN_TLO_PARSING_DEV_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
