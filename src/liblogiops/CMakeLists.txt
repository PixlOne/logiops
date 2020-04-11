cmake_minimum_required(VERSION 3.10)
project(liblogiops)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../CMake")

set(LIBLOGIOPS_SOURCES
        liblogiops_client/Socket.cpp)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
find_package(DBus1 REQUIRED)
include_directories(${DBUS_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})

include_directories(.)

add_library(liblogiops SHARED ${LIBLOGIOPS_SOURCES})

target_link_libraries(liblogiops ${DBUS_LIBRARIES} ${GLIB_LDFLAGS})

set_target_properties(liblogiops PROPERTIES PREFIX "")

install(DIRECTORY "./" DESTINATION "include/liblogiops"
        FILES_MATCHING PATTERN "*.h")
install(TARGETS liblogiops
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)