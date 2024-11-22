if(COMPILE_RUNTIME_LIGHT)
  set(ZLIB_BUILD_EXAMPLES OFF BOOL "Disable ZLIB_BUILD_EXAMPLES")
  set(ZLIB_COMPILE_FLAGS "-O3" "-fPIC")

  set(RENAME_ZCONF OFF)
  add_subdirectory(${THIRD_PARTY_DIR}/zlib ${CMAKE_BINARY_DIR}/third-party/zlib)

  target_compile_definitions(zlibstatic PRIVATE Z_HAVE_UNISTD_H)
  target_compile_definitions(zlib PRIVATE Z_HAVE_UNISTD_H)
  target_compile_options(zlibstatic PUBLIC ${ZLIB_COMPILE_FLAGS})

  # Set output directories for zlib targets
  set(ZLIB_LIB_DIR "${OBJS_DIR}/lib")
  set_target_properties(zlibstatic PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
                                              ${ZLIB_LIB_DIR})
endif()
