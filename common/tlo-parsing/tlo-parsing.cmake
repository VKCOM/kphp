include_guard(GLOBAL)

prepend(TLO_PARSING_OBJECTS ${COMMON_DIR}/tlo-parsing/
        flat-optimization.cpp
        remove-odd-types.cpp
        replace-anonymous-args.cpp
        tl-dependency-graph.cpp
        tl-objects.cpp
        tl-scheme-final-check.cpp
        tlo-parser.cpp)

vk_add_library(tlo_parsing_src OBJECT ${TLO_PARSING_OBJECTS})

set_target_properties(tlo_parsing_src PROPERTIES POSITION_INDEPENDENT_CODE 1)

prepend(TLO_PARSING_PUBLIC_HEADERS ${COMMON_DIR}/tlo-parsing/
        tl-objects.h
        tl-dependency-graph.h
        tlo-parsing.h)

vk_add_library(tlo_parsing_static STATIC $<TARGET_OBJECTS:tlo_parsing_src>)
set_target_properties(tlo_parsing_static PROPERTIES
        OUTPUT_NAME tlo_parsing
        PUBLIC_HEADER "${TLO_PARSING_PUBLIC_HEADERS}")

install(TARGETS tlo_parsing_static
        COMPONENT TLO_PARSING_DEV
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION /usr/${CMAKE_INSTALL_INCLUDEDIR}/tlo-parsing)

vk_add_library(tlo_parsing_shared SHARED $<TARGET_OBJECTS:tlo_parsing_src>)
set_target_properties(tlo_parsing_shared PROPERTIES OUTPUT_NAME tlo_parsing)

if(NOT APPLE)
    target_link_options(tlo_parsing_shared PRIVATE -Wl,--version-script=${COMMON_DIR}/tlo-parsing/tlo-parsing-lib-exported-symbols.map)
endif()

install(TARGETS tlo_parsing_shared
        COMPONENT TLO_PARSING
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(CPACK_DEBIAN_TLO_PARSING_PACKAGE_NAME "libtlo-parsing")
set(CPACK_DEBIAN_TLO_PARSING_DESCRIPTION "Library files for the tlo parsing")

set(CPACK_DEBIAN_TLO_PARSING_DEV_PACKAGE_NAME "libtlo-parsing-dev")
set(CPACK_DEBIAN_TLO_PARSING_DEV_DESCRIPTION "Development files and headers for the tlo parsing")
set(CPACK_DEBIAN_TLO_PARSING_DEV_PACKAGE_DEPENDS "libtlo-parsing")
