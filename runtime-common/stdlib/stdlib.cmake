prepend(STDLIB_ARRAY stdlib/array/ array-functions.cpp)
prepend(STDLIB_STRING stdlib/string/ json-functions.cpp
                                     json-writer.cpp
                                     mbstring-functions.cpp
                                     string-functions.cpp)
prepend(STDLIB_SERVER stdlib/server/ url-functions.cpp)

set(STDLIB_SRC ${STDLIB_ARRAY} ${STDLIB_STRING} ${STDLIB_SERVER})
