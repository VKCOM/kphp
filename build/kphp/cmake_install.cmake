# Install script for directory: /opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/../install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/opt/KasperskyOS-Community-Edition-1.1.1.13/toolchain/bin/aarch64-kos-objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-build/cmake_install.cmake")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xFLEXx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/lib" TYPE STATIC_LIBRARY FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/objs/flex/libvk-flex-data.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xFLEXx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/lib" TYPE STATIC_LIBRARY FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/objs/flex/libvk-flex-data.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xVKEXT7.4x" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/../install/usr/lib/php/20190902/vkext.so")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/../install/usr/lib/php/20190902" TYPE FILE FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/objs/vkext/modules7.4/vkext.so")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xVKEXT7.4x" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/etc/php/7.4/mods-available" TYPE FILE FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/vkext/vkext.ini")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xVKEXT7.4x" OR NOT CMAKE_INSTALL_COMPONENT)
  
            file(MAKE_DIRECTORY $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/etc/php/7.4/apache2/conf.d)
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_INSTALL_PREFIX}/etc/php/7.4/mods-available/vkext.ini 20-vkext.ini
                WORKING_DIRECTORY $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/etc/php/7.4/apache2/conf.d
            )
        
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xVKEXT7.4x" OR NOT CMAKE_INSTALL_COMPONENT)
  
            file(MAKE_DIRECTORY $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/etc/php/7.4/cli/conf.d)
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_INSTALL_PREFIX}/etc/php/7.4/mods-available/vkext.ini 20-vkext.ini
                WORKING_DIRECTORY $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/etc/php/7.4/cli/conf.d
            )
        
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xTLO_PARSING_DEVx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/kphp/libtlo_parsing.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xTLO_PARSING_DEVx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/include/tlo-parsing" TYPE FILE FILES
    "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/common/tlo-parsing/tl-objects.h"
    "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/common/tlo-parsing/tl-dependency-graph.h"
    "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/common/tlo-parsing/tlo-parsing.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xTLO_PARSINGx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/kphp/libtlo_parsing.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xTL_TOOLSx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/objs/bin/tl-compiler")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/tl-compiler" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/tl-compiler")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/opt/KasperskyOS-Community-Edition-1.1.1.13/toolchain/bin/aarch64-kos-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/tl-compiler")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xTL_TOOLSx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/objs/bin/tl2php")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/tl2php" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/tl2php")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/opt/KasperskyOS-Community-Edition-1.1.1.13/toolchain/bin/aarch64-kos-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/tl2php")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xTL_TOOLSx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/share/vkontakte/examples" TYPE DIRECTORY FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/common/tl-files" FILES_MATCHING REGEX "/[^/]*\\.tl$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xnk_headersx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/kphp" TYPE FILE FILES
    "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_headers_/kphp/../kphp/Kphp.edl"
    "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_headers_/kphp/../kphp/Kphp.edl.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xKPHPx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/share/vkontakte/bin" TYPE EXECUTABLE FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/objs/bin/kphp2cpp")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/usr/share/vkontakte/bin/kphp2cpp" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/usr/share/vkontakte/bin/kphp2cpp")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/opt/KasperskyOS-Community-Edition-1.1.1.13/toolchain/bin/aarch64-kos-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/usr/share/vkontakte/bin/kphp2cpp")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xKPHPx" OR NOT CMAKE_INSTALL_COMPONENT)
  
            file(MAKE_DIRECTORY $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin)
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_INSTALL_PREFIX}/usr/share/vkontakte/bin/kphp2cpp kphp
                WORKING_DIRECTORY $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin
            )
        
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xKPHPx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/share/vkontakte/kphp_source/objs" TYPE STATIC_LIBRARY FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/objs/libkphp-full-runtime.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xKPHPx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/share/vkontakte/kphp_source" TYPE DIRECTORY FILES
    "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/common"
    "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/runtime"
    "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/server"
    FILES_MATCHING REGEX ".*\\.(h|inl)$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xKPHPx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/share/vkontakte/kphp_source/objs" TYPE FILE FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/objs/php_lib_version.sha256")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xKPHPx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/share/vkontakte/kphp_source/" TYPE DIRECTORY FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/builtin-functions")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xKPHPx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/share/vkontakte/kphp_source" TYPE FILE FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/common/php-functions.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xKPHPx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/share/vkontakte/kphp_source/objs/generated/auto/runtime" TYPE FILE FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/kphp/objs/generated/auto/runtime/runtime-headers.h")
endif()

