prepend(STDLIB_ARRAY stdlib/array/ array-functions.cpp)
prepend(STDLIB_MATH stdlib/math/ math-functions.cpp)
prepend(STDLIB_SERIALIZATION stdlib/serialization/ json-functions.cpp
        json-writer.cpp serialize-functions.cpp)
prepend(STDLIB_STRING stdlib/string/ mbstring-functions.cpp
        string-functions.cpp)
prepend(STDLIB_SERVER stdlib/server/ url-functions.cpp)

set(STDLIB_SRC ${STDLIB_ARRAY} ${STDLIB_MATH} ${STDLIB_SERIALIZATION}
               ${STDLIB_STRING} ${STDLIB_SERVER})
