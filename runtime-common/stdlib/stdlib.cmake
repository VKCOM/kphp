prepend(STDLIB_ARRAY stdlib/array/ array-functions.cpp)
prepend(STDLIB_HASH stdlib/hash/ hash-functions.cpp)
prepend(STDLIB_MATH stdlib/math/ math-functions.cpp
        bcmath-functions.cpp math-context.cpp)
prepend(STDLIB_SERIALIZATION stdlib/serialization/ json-functions.cpp
        json-writer.cpp serialize-functions.cpp)
prepend(STDLIB_STRING stdlib/string/ mbstring-functions.cpp
        regex-functions.cpp string-functions.cpp)
prepend(STDLIB_SERVER stdlib/server/ url-functions.cpp)
prepend(STDLIB_VKEXT stdlib/vkext/ string-processing.cpp
        vkext-functions.cpp)

if(COMPILER_CLANG)
    set_source_files_properties(${RUNTIME_COMMON_DIR}/stdlib/vkext/string-processing.cpp PROPERTIES COMPILE_FLAGS -Wno-invalid-source-encoding)
endif()

set(STDLIB_SRC ${STDLIB_ARRAY} ${STDLIB_HASH} ${STDLIB_MATH} ${STDLIB_SERIALIZATION}
               ${STDLIB_STRING} ${STDLIB_SERVER} ${STDLIB_VKEXT})
