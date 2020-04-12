cmake_minimum_required(VERSION 3.10)
project(liblogiops)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(LIBLOGIOPS_SOURCES
        liblogiops_client/Socket.cpp)

find_package(PkgConfig REQUIRED)
find_package(DBusCXX REQUIRED)
include_directories(${DBUSCXX_INCLUDE_DIR})

include_directories(.)

add_library(liblogiops SHARED ${LIBLOGIOPS_SOURCES})

target_link_libraries(liblogiops ${DBUSCXX_LIBRARIES})

set_target_properties(liblogiops PROPERTIES PREFIX "")

install(DIRECTORY "./" DESTINATION "include/liblogiops"
        FILES_MATCHING PATTERN "*.h")
install(TARGETS liblogiops
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)