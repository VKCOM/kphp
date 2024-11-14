if(COMPILE_RUNTIME_LIGHT)
  set(ZLIB_BUILD_EXAMPLES OFF)
  set(ZLIB_COMPILE_FLAGS "-O3" "-fPIC")

  add_subdirectory(${THIRD_PARTY_DIR}/zlib ${CMAKE_BINARY_DIR}/third-party/zlib)
  # this is a hack to prevent zconf.h from being renamed to zconf.h.included
  file(RENAME ${THIRD_PARTY_DIR}/zlib/zconf.h.included
       ${THIRD_PARTY_DIR}/zlib/zconf.h)
  target_compile_options(zlibstatic PUBLIC ${ZLIB_COMPILE_FLAGS})
  # Set output directories for zlib targets
  set_target_properties(zlibstatic PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
                                              "${OBJS_DIR}")
  set(ZLIB_LIB_DIR "${OBJS_DIR}")
endif()
