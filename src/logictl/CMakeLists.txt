cmake_minimum_required(VERSION 3.10)
project(logictl)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(DBusCXX REQUIRED)

add_executable(logictl
        logictl.cpp
        client/Control.cpp
        client/Socket.cpp)

set_target_properties(logictl PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories(${DBUSCXX_INCLUDE_DIR})

target_link_libraries(logictl ${CMAKE_THREAD_LIBS_INIT} ${DBUSCXX_LIBRARIES})

install(TARGETS logictl DESTINATION bin)
