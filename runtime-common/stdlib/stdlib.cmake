prepend(STDLIB_ARRAY stdlib/array/ array-functions.cpp)
prepend(STDLIB_CRYPTO stdlib/crypto/ crypto-functions.cpp)
prepend(STDLIB_MATH stdlib/math/ math-functions.cpp
        bcmath-functions.cpp math-context.cpp)
prepend(STDLIB_MSGPACK stdlib/msgpack/ object_visitor.cpp
        packer.cpp parser.cpp unpacker.cpp zone.cpp)
prepend(STDLIB_RPC stdlib/rpc/ rpc-parse.cpp)
prepend(STDLIB_SERIALIZATION stdlib/serialization/ json-functions.cpp
        json-writer.cpp serialize-functions.cpp)
prepend(STDLIB_STRING stdlib/string/ mbstring-functions.cpp
        regex-functions.cpp string-functions.cpp)
prepend(STDLIB_SYSTEM stdlib/system/ system-functions.cpp)
prepend(STDLIB_SERVER stdlib/server/ url-functions.cpp
        net-functions.cpp)
prepend(STDLIB_VKEXT stdlib/vkext/ string-processing.cpp
        vkext-functions.cpp vkext-stats.cpp)

if(COMPILER_CLANG)
    set_source_files_properties(${RUNTIME_COMMON_DIR}/stdlib/vkext/string-processing.cpp PROPERTIES COMPILE_FLAGS -Wno-invalid-source-encoding)
endif()

set(STDLIB_SRC ${STDLIB_ARRAY} ${STDLIB_CRYPTO} ${STDLIB_MATH} ${STDLIB_MSGPACK} ${STDLIB_RPC} ${STDLIB_SERIALIZATION}
               ${STDLIB_STRING} ${STDLIB_SYSTEM} ${STDLIB_SERVER} ${STDLIB_VKEXT})
