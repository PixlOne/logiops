cmake_minimum_required(VERSION 3.10)
project(logictl)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
find_package(DBus REQUIRED)

add_executable(logictl
        logictl.cpp)

set_target_properties(logictl PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories(${DBUS_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} ../liblogiops)

target_link_libraries(logictl liblogiops ${CMAKE_THREAD_LIBS_INIT} ${DBUS_LIBRARIES} ${GLIB_LDFLAGS})

install(TARGETS logictl DESTINATION bin)
