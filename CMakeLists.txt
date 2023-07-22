cmake_minimum_required (VERSION 3.12)

project (kphp)

# Initialize CMake library for KasperskyOS SDK.
include (platform)
initialize_platform (FORCE_STATIC)

add_subdirectory (kphp)
add_subdirectory (einit)
