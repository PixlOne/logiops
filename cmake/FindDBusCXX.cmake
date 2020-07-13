# - Try to find the D-Bus C++ library
# Once done this will define
#
#  DBUSCXX_FOUND - system has D-Bus C++
#  DBUSCXX_INCLUDE_DIR - the D-Bus C++ include directory
#  DBUSCXX_LIBRARIES - the libraries needed to use D-Bus C++
#  DBUSCXX_DEFINITIONS - Compiler switches required for using D-Bus C++

# Copyright (c) 2012 Ni Hui, <shuizhuyuanluo@126.com>
# Based on Laurent Montel's FindFontConfig.cmake, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (DBUSCXX_INCLUDE_DIR AND DBUSCXX_LIBRARIES)

    # in cache already
    set(DBUSCXX_FOUND TRUE)

else (DBUSCXX_INCLUDE_DIR AND DBUSCXX_LIBRARIES)

    if (NOT WIN32)
        # use pkg-config to get the directories and then use these values
        # in the FIND_PATH() and FIND_LIBRARY() calls
        find_package(PkgConfig)
        pkg_check_modules(PC_DBUSCXX QUIET dbus-c++-1)

        set(DBUSCXX_DEFINITIONS ${PC_DBUSCXX_CFLAGS_OTHER})
    endif (NOT WIN32)

    find_path(DBUSCXX_INCLUDE_DIR dbus.h
        PREFIX dbus-c++
        PATHS
        ${PC_DBUSCXX_INCLUDEDIR}
        ${PC_DBUSCXX_INCLUDE_DIRS}
    )

   find_library(DBUSCXX_LIBRARIES NAMES dbus-c++-1
        PATHS
        ${PC_DBUSCXX_LIBDIR}
        ${PC_DBUSCXX_LIBRARY_DIRS}
   )

   include(FindPackageHandleStandardArgs)
   FIND_PACKAGE_HANDLE_STANDARD_ARGS(DBusCXX DEFAULT_MSG DBUSCXX_LIBRARIES DBUSCXX_INCLUDE_DIR)

   mark_as_advanced(DBUSCXX_LIBRARIES DBUSCXX_INCLUDE_DIR)
endif (DBUSCXX_INCLUDE_DIR AND DBUSCXX_LIBRARIES)

find_program(DBUSXX_XML2CPP_EXECUTABLE NAMES dbusxx-xml2cpp)

macro(dbuscxx_add_dbus_adaptor _sources _xml_file)
    get_filename_component(_infile ${_xml_file} ABSOLUTE)
    string(REGEX REPLACE "(.*[/\\.])?([^\\.]+)\\.xml" "\\2_adaptor" _basename ${_infile})
    string(TOLOWER ${_basename} _basename)
    set(_header ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.h)
    add_custom_command(OUTPUT ${_header}
        COMMAND ${DBUSXX_XML2CPP_EXECUTABLE} ${_infile} --adaptor=${_header}
        DEPENDS ${_infile}
        COMMENT "Generating ${_basename}.h"
    )
    set(${_sources} ${${_sources}} ${_header})
endmacro(dbuscxx_add_dbus_adaptor)

macro(dbuscxx_add_dbus_proxy _sources _xml_file)
    get_filename_component(_infile ${_xml_file} ABSOLUTE)
    string(REGEX REPLACE "(.*[/\\.])?([^\\.]+)\\.xml" "\\2_proxy" _basename ${_infile})
    string(TOLOWER ${_basename} _basename)
    set(_header ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.h)
    add_custom_command(OUTPUT ${_header}
        COMMAND ${DBUSXX_XML2CPP_EXECUTABLE} ${_infile} --proxy=${_header}
        DEPENDS ${_infile}
        COMMENT "Generating ${_basename}.h"
    )
    set(${_sources} ${${_sources}} ${_header})
endmacro(dbuscxx_add_dbus_proxy)
