include_guard(GLOBAL)

# Light runtime require c++20
if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    if (COMPILE_RUNTIME_LIGHT)
        check_compiler_version(clang 14.0.0)
    else()
        check_compiler_version(clang 10.0.0)
    endif()
    set(COMPILER_CLANG True)
elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    if (COMPILE_RUNTIME_LIGHT)
        check_compiler_version(gcc 10.1.0)
    else()
        check_compiler_version(gcc 8.3.0)
    endif()
    set(COMPILER_GCC True)
endif()

if(COMPILE_RUNTIME_LIGHT)
    set(REQUIRED_CMAKE_CXX_STANDARD 23)
    if(RUNTIME_LIGHT_HIDDEN_VISIBILITY)
        set(RUNTIME_LIGHT_VISIBILITY -fvisibility=hidden)
    else()
        set(RUNTIME_LIGHT_VISIBILITY -fvisibility=default)
    endif()
else()
    set(REQUIRED_CMAKE_CXX_STANDARD 17)
endif()

set(CMAKE_CXX_STANDARD ${REQUIRED_CMAKE_CXX_STANDARD} CACHE STRING "C++ standard to conform to")
set(CMAKE_CXX_EXTENSIONS OFF)
if (CMAKE_CXX_STANDARD LESS ${REQUIRED_CMAKE_CXX_STANDARD})
    message(FATAL_ERROR "c++${REQUIRED_CMAKE_CXX_STANDARD} expected at least!")
endif()

cmake_print_variables(CMAKE_CXX_STANDARD)


if(APPLE)
    add_definitions(-D_XOPEN_SOURCE)
else()
    # Since Ubuntu 22.04 lto is enabled by default; breaks some builds
    #add_link_options(-fno-lto)
endif()


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
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION 1)
    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
        set(COMPILE_LTO_OPTIONS -flto)
        set(LINK_LTO_OPTIONS -flto)
        if(NOT APPLE)
            list(APPEND COMPILE_LTO_OPTIONS -ffat-lto-objects)
        endif()
        if(COMPILER_GCC)
            list(APPEND COMPILE_LTO_OPTIONS -Wl,-flto)
        endif()
        add_compile_options(${COMPILE_LTO_OPTIONS})
        add_link_options(${LINK_LTO_OPTIONS})
        list(JOIN COMPILE_LTO_OPTIONS " " THIRD_PARTY_LTO_OPTIONS)
    endif()
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

add_compile_options(-Werror -Wall -Wextra -Wunused-function -Wfloat-conversion -Wno-sign-compare
                    -Wuninitialized -Wno-redundant-move -Wno-missing-field-initializers)

if(CMAKE_CXX_COMPILER_ID MATCHES Clang AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "17")
    add_compile_options(-Wno-vla-cxx-extension)
endif()

if(COMPILE_RUNTIME_LIGHT)
    add_compile_options(-Wno-vla-extension)
endif()

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
if(COMPILE_RUNTIME_LIGHT)
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
endif()
