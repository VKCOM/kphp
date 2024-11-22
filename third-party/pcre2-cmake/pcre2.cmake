if(COMPILE_RUNTIME_LIGHT)
  # set cmake_policy to silence set & option warnings
  cmake_policy(PUSH)
  cmake_policy(SET CMP0077 NEW)

  set(PCRE2_STATIC_PIC
      ON
      CACHE BOOL "Enable PCRE2 PIC")
  set(PCRE2_BUILD_PCRE2_16
      OFF
      CACHE BOOL "Disable PCRE2-16")
  set(PCRE2_BUILD_PCRE2_32
      OFF
      CACHE BOOL "Disable PCRE2-32")
  set(PCRE2_SUPPORT_JIT
      ON
      CACHE BOOL "Enable PCRE2 JIT")
  set(PCRE2_BUILD_PCRE2GREP
      OFF
      CACHE BOOL "Disable build of pcre2grep")
  set(PCRE2_BUILD_TESTS
      OFF
      CACHE BOOL "Disable build of PCRE2 tests")
  set(PCRE2_SUPPORT_LIBBZ2
      OFF
      CACHE BOOL "Disable PCRE2 LIBBZ2 support")
  set(PCRE2_SUPPORT_LIBZ
      OFF
      CACHE BOOL "Disable PCRE2 ZLIB support")
  set(PCRE2_SUPPORT_LIBEDIT
      OFF
      CACHE BOOL "Disable PCRE2 LIBEDIT support")
  set(PCRE2_SUPPORT_LIBREADLINE
      OFF
      CACHE BOOL "Disable PCRE2 LIBREADLINE support")

  # set the output directory for static lib
  set(PCRE2_LIB_DIR "${OBJS_DIR}/lib")
  set(SAVE_CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PCRE2_LIB_DIR})

  # set cmake_install_prefix
  set(PCRE2_INSTALL_DIR "${OBJS_DIR}")
  set(SAVE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
  set(CMAKE_INSTALL_PREFIX ${PCRE2_INSTALL_DIR})

  # save and set C flags
  set(SAVE_C_FLAGS "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")

  add_subdirectory(${THIRD_PARTY_DIR}/pcre2
                   ${CMAKE_BINARY_DIR}/third-party/pcre2)

  # restore C flags
  set(CMAKE_C_FLAGS ${SAVE_C_FLAGS})

  # restore cmake_install prefix
  set(CMAKE_INSTALL_PREFIX "${SAVE_INSTALL_PREFIX}")

  # restore CMAKE_ARCHIVE_OUTPUT_DIRECTORY
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SAVE_CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")

  # restore cmake_policy
  cmake_policy(POP)
endif()
