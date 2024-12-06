if(COMPILE_RUNTIME_LIGHT)
  set(ABSL_ENABLE_INSTALL OFF BOOL "Disable ABSL install")
  set(ABSL_FIND_GOOGLETEST OFF BOOL "Use already installed GTest for ABSL")
  set(ABSL_PROPAGATE_CXX_STD ON BOOL)
  # Set the output directory for static lib
  set(ABSEIL_LIB_DIR "${OBJS_DIR}/lib")
  set(SAVE_CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ABSEIL_LIB_DIR})
  # set compiler flags
  set(SAVE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(ABSL_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fPIC -stdlib=libc++")
  set(CMAKE_CXX_FLAGS ${ABSL_CXX_FLAGS})

  add_subdirectory(${THIRD_PARTY_DIR}/abseil-cpp
                   ${CMAKE_BINARY_DIR}/third-party/abseil-cpp)
  # disable absl logging
  add_compile_definitions(ABSL_MIN_LOG_LEVEL=4)
  # restore compiler flags
  set(CMAKE_CXX_FLAGS ${SAVE_CXX_FLAGS})

  # restore CMAKE_ARCHIVE_OUTPUT_DIRECTORY
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SAVE_CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")

  # due to "efficiency-first" approach in abseil we need to manually specify all
  # the static libraries we want to link against
  set(ABSEIL_LIBS libabsl_base.a libabsl_strings.a
                  libabsl_raw_logging_internal.a)
  prepend(ABSEIL_LIBS ${ABSEIL_LIB_DIR}/ ${ABSEIL_LIBS})
endif()
