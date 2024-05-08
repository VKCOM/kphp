include_guard(GLOBAL)

# TODO move all runtime stuff in runtime-light.cmake
# Because we do not need coroutines for compiler
# Or we do not need it?

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    check_compiler_version(clang 14.0.0)
    set(COMPILER_CLANG True)
elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    add_compile_options(-fcoroutines)
    check_compiler_version(gcc 10.1.0)
    set(COMPILER_GCC True)
endif()

set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard to conform to")
set(CMAKE_CXX_EXTENSIONS OFF)
if (CMAKE_CXX_STANDARD LESS 20)
    message(FATAL_ERROR "c++20 expected at least!")
endif()
cmake_print_variables(CMAKE_CXX_STANDARD)

if(APPLE)
    add_definitions(-D_XOPEN_SOURCE)
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        include_directories(/usr/local/include)
        set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl" CACHE INTERNAL "")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        include_directories(/opt/homebrew/include)
        link_directories(/opt/homebrew/lib)
        set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl" CACHE INTERNAL "")
    else()
        message(FATAL_ERROR "unsupported arch: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
else()
    # Since Ubuntu 22.04 lto is enabled by default; breaks some builds
    add_link_options(-fno-lto)
endif()

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

option(ADDRESS_SANITIZER "Enable address sanitizer")
if(ADDRESS_SANITIZER)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
    add_definitions(-DASAN_ENABLED=1)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (default Debug)" FORCE)
endif()

option(UNDEFINED_SANITIZER "Enable undefined sanitizer")
if(UNDEFINED_SANITIZER)
    add_compile_options(-fsanitize=undefined -fno-sanitize-recover=all)
    add_link_options(-fsanitize=undefined)
    add_definitions(-DUSAN_ENABLED=1)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (default Debug)" FORCE)
endif()
cmake_print_variables(ADDRESS_SANITIZER UNDEFINED_SANITIZER)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to `${DEFAULT_BUILD_TYPE}` as none was specified.")
    set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE} CACHE STRING "Build type (default ${DEFAULT_BUILD_TYPE})" FORCE)
endif()

cmake_print_variables(CMAKE_BUILD_TYPE)
if("${CMAKE_BUILD_TYPE}" STREQUAL ${DEFAULT_BUILD_TYPE})
    add_compile_options(-O3)
endif()

if(DEFINED ENV{FAST_COMPILATION_FMT})
    add_definitions(-DFAST_COMPILATION_FMT)
    message(STATUS FAST_COMPILATION_FMT="ON")
endif()

if(NOT DEFINED ENV{ENABLE_GRPROF})
    add_compile_options(-fdata-sections -ffunction-sections)
    if(APPLE)
        add_link_options(-Wl,-dead_strip)
    else()
        add_link_options(-Wl,--gc-sections)
    endif()
endif()

include_directories(${GENERATED_DIR})
add_compile_options(-fwrapv -fno-strict-aliasing -fno-stack-protector -ggdb -fno-omit-frame-pointer -fno-common -fsigned-char)
add_link_options(-fno-common)
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    add_compile_options(-march=sandybridge)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    add_compile_options(-march=armv8.2-a+crypto)
endif()

add_compile_options(-Wall -Wextra -Wunused-function -Wfloat-conversion -Wno-sign-compare
                    -Wuninitialized -Wno-redundant-move -Wno-missing-field-initializers)

if(NOT APPLE)
    check_cxx_compiler_flag(-gz=zlib DEBUG_COMPRESSION_IS_FOUND)
    if (DEBUG_COMPRESSION_IS_FOUND)
        add_compile_options(-gz=zlib)
        add_link_options(-Wl,--compress-debug-sections=zlib)
    endif()
endif()

add_link_options(-rdynamic -L/usr/local/lib -ggdb)
add_definitions(-D_GNU_SOURCE)
# prevents the `build` directory to be appeared in symbols, it's necessary for remote debugging with path mappings
add_compile_options(-fdebug-prefix-map="${CMAKE_BINARY_DIR}=${CMAKE_SOURCE_DIR}")

# Light runtime uses C++20 coroutines heavily, so they are required
get_directory_property(TRY_COMPILE_COMPILE_OPTIONS COMPILE_OPTIONS)
string (REPLACE ";" " " TRY_COMPILE_COMPILE_OPTIONS "${TRY_COMPILE_COMPILE_OPTIONS}")
file(WRITE "${PROJECT_BINARY_DIR}/check_coroutine_include.cpp"
        "#include<coroutine>\n"
        "int main() {}\n")
try_compile(
        HAS_COROUTINE
        "${PROJECT_BINARY_DIR}/tmp"
        "${PROJECT_BINARY_DIR}/check_coroutine_include.cpp"
        COMPILE_DEFINITIONS "${TRY_COMPILE_COMPILE_OPTIONS}"
)
if(NOT HAS_COROUTINE)
    message(FATAL_ERROR "Compiler or libstdc++ does not support coroutines")
endif()
file(REMOVE "${PROJECT_BINARY_DIR}/check_coroutine_include.cpp")
