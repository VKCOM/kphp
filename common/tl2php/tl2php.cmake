prepend(TL2PHP_SOURCES ${COMMON_DIR}/tl2php/
        tl2php.cpp
        combinator-to-php.cpp
        constructor-to-php.cpp
        function-to-php.cpp
        gen-php-code.cpp
        gen-php-tests.cpp
        php-classes.cpp
        tl-hints.cpp
        tl-to-php-classes-converter.cpp)

add_executable(tl2php ${TL2PHP_SOURCES})
target_link_libraries(tl2php PRIVATE pthread vk::${PIC_MODE}::popular-common vk::${PIC_MODE}::tlo_parsing_src)
target_link_options(tl2php PRIVATE ${PIE_MODE_FLAGS})
set_target_properties(tl2php PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
