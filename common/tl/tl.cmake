include(${COMMON_DIR}/tlo-parsing/tlo-parsing.cmake)
include(${COMMON_DIR}/tl2php/tl2php.cmake)

install(TARGETS tl2php
        COMPONENT TL_TOOLS
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY ${COMMON_DIR}/tl-files
        COMPONENT TL_TOOLS
        DESTINATION ${VK_INSTALL_DIR}/examples
        FILES_MATCHING PATTERN "*.tl")

set(CPACK_DEBIAN_TL_TOOLS_PACKAGE_NAME "vk-tl-tools")
set(CPACK_DEBIAN_TL_TOOLS_DESCRIPTION "Tools for processing tl files")
