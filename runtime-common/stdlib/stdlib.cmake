prepend(STDLIB_STRING stdlib/string/ json-functions.cpp
                                     json-writer.cpp
                                     mbstring-functions.cpp
                                     string-functions.cpp)
prepend(STDLIB_SERVER stdlib/server/ url-functions.cpp)
prepend(STDLIB_VKEXT stdlib/vkext/ vkext.cpp vkext_stats.cpp string-processing.cpp)

if(COMPILER_CLANG)
  set_source_files_properties(${RUNTIME_COMMON_DIR}/stdlib/vkext/string-processing.cpp PROPERTIES COMPILE_FLAGS -Wno-invalid-source-encoding)
endif()

set(STDLIB_SRC ${STDLIB_STRING} ${STDLIB_SERVER} ${STDLIB_VKEXT})
