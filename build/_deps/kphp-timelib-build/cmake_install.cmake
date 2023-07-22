# Install script for directory: /opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-src

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

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xKPHPx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/../install/lib/libkphp-timelib.a")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/../install/lib" TYPE STATIC_LIBRARY FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-src/objs/libkphp-timelib.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xKPHPx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/../install/include/kphp/timelib/timelib.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/../install/include/kphp/timelib" TYPE FILE FILES "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-src/timelib.h")
endif()

