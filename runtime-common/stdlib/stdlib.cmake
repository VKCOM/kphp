prepend(STDLIB_STRING stdlib/string/ string-functions.cpp
        mbstring-functions.cpp)
prepend(STDLIB_MATH stdlib/math/ math-functions.cpp)

set(STDLIB_SRC ${STDLIB_STRING} ${STDLIB_MATH})
