include_guard(GLOBAL)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to conform to")
set(CMAKE_CXX_EXTENSIONS OFF)
if (CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "c++17 expected at least!")
endif()
cmake_print_variables(CMAKE_CXX_STANDARD)

#add_compile_options(-Werror -Wall -Wextra -Wunused-function -Wfloat-conversion -Wno-sign-compare
#                    -Wuninitialized -Wno-redundant-move -Wno-missing-field-initializers)

