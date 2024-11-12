prepend(STDLIB_STRING stdlib/string/ string-functions.cpp
        mbstring-functions.cpp)
prepend(STDLIB_SERVER stdlib/server/ url-functions.cpp)

set(STDLIB_SRC ${STDLIB_STRING} ${STDLIB_SERVER})
