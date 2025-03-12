update_git_submodule(${THIRD_PARTY_DIR}/pcre2 "--recursive")
get_submodule_version(${THIRD_PARTY_DIR}/pcre2 PCRE2_VERSION)
get_submodule_remote_url(third-party/pcre2 PCRE2_SOURCE_URL)

set(PCRE2_PROJECT_GENERIC_NAME pcre2)
set(PCRE2_PROJECT_GENERIC_NAMESPACE PCRE2)
set(PCRE2_ARTIFACT_PREFIX libpcre2)

function(build_pcre2 PIC_ENABLED)
  make_third_party_configuration(
    ${PIC_ENABLED}
    ${PCRE2_PROJECT_GENERIC_NAME}
    ${PCRE2_PROJECT_GENERIC_NAMESPACE}
    project_name
    target_name
    extra_compile_flags
    pic_namespace
    pic_lib_specifier)

  set(source_dir ${THIRD_PARTY_DIR}/${PCRE2_PROJECT_GENERIC_NAME})
  set(build_dir ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
  set(install_dir ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
  set(include_dirs ${install_dir}/include)
  set(libraries ${install_dir}/lib/${PCRE2_ARTIFACT_PREFIX}-8.a)
  # Ensure the build, installation and "include" directories exists
  file(MAKE_DIRECTORY ${build_dir})
  file(MAKE_DIRECTORY ${install_dir})
  file(MAKE_DIRECTORY ${include_dirs})

  set(compile_flags "$ENV{CFLAGS} -O3 ${extra_compile_flags}")

  message(
    STATUS
      "PCRE2 Summary:

        PIC enabled:    ${PIC_ENABLED}
	Version:        ${PCRE2_VERSION}
	Source:         ${PCRE2_SOURCE_URL}
        Include dirs:   ${include_dirs}
        Libraries:      ${libraries}
        Target name:    ${target_name}
        Compiler:
          C compiler:   ${CMAKE_C_COMPILER}
          CFLAGS:       ${compile_flags}
    ")

  set(PCRE2_SUPPORT_JIT)
  if(APPLE)
    set(PCRE2_SUPPORT_JIT OFF BOOL)
  else()
    set(PCRE2_SUPPORT_JIT ON BOOL)
  endif()

  set(cmake_args
      -DCMAKE_C_FLAGS=${compile_flags}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DBUILD_STATIC_LIBS=ON
      -DCMAKE_POSITION_INDEPENDENT_CODE=${PIC_ENABLED}
      -DPCRE2_STATIC_PIC=${PIC_ENABLED}
      -DPCRE2_BUILD_PCRE2_16=OFF
      -DPCRE2_BUILD_PCRE2_32=OFF
      -DPCRE2_DEBUG=OFF
      -DPCRE2_SUPPORT_JIT=${PCRE2_SUPPORT_JIT}
      -DPCRE2_SUPPORT_UNICODE=OFF
      -DPCRE2_BUILD_PCRE2GREP=OFF
      -DPCRE2_BUILD_TESTS=OFF
      -DPCRE2_SUPPORT_LIBBZ2=OFF
      -DPCRE2_SUPPORT_LIBZ=OFF
      -DPCRE2_SUPPORT_LIBEDIT=OFF
      -DPCRE2_SUPPORT_LIBREADLINE=OFF)

  ExternalProject_Add(
    ${project_name}
    PREFIX ${build_dir}
    SOURCE_DIR ${source_dir}
    INSTALL_DIR ${install_dir}
    BINARY_DIR ${build_dir}
    BUILD_BYPRODUCTS ${libraries}
    CONFIGURE_COMMAND
    COMMAND ${CMAKE_COMMAND} ${cmake_args} -S ${source_dir} -B ${build_dir}
    BUILD_COMMAND
    COMMAND ${CMAKE_COMMAND} --build ${build_dir} --config $<CONFIG> -j
    INSTALL_COMMAND
    COMMAND ${CMAKE_COMMAND} --install ${build_dir} --prefix ${install_dir} --config $<CONFIG>
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${include_dirs} ${include_dirs}/${PCRE2_PROJECT_GENERIC_NAME}
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory
      ${include_dirs}/${PCRE2_PROJECT_GENERIC_NAME}
      ${INCLUDE_DIR}/${PCRE2_PROJECT_GENERIC_NAME}
    BUILD_IN_SOURCE 0)

  add_library(${target_name} STATIC IMPORTED)
  set_target_properties(${target_name} PROPERTIES IMPORTED_LOCATION ${libraries} INTERFACE_INCLUDE_DIRECTORIES ${include_dirs})

  # Ensure that the pcre2 is built before they are used
  add_dependencies(${target_name} ${project_name})

  # Set variables indicating that pcre2 has been installed
  set(${PCRE2_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_ROOT ${install_dir} PARENT_SCOPE)
  set(${PCRE2_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
  set(${PCRE2_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_LIBRARIES ${libraries} PARENT_SCOPE)
endfunction()

# PIC is ON
build_pcre2(ON)

set(PCRE2_FOUND ON)
