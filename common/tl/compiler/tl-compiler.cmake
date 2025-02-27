set(TL_COMPILER_DIR ${COMMON_DIR}/tl/compiler)

set(TL_COMPILER_SOURCES ${TL_COMPILER_DIR}/tl-compiler.cpp ${TL_COMPILER_DIR}/tl-parser-new.cpp)
add_executable(tl-compiler ${TL_COMPILER_SOURCES})
target_link_libraries(tl-compiler PRIVATE vk::${PIC_MODE}::popular-common pthread)
target_link_options(tl-compiler PRIVATE ${PIE_MODE_FLAGS})
set_target_properties(tl-compiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
