cmake_minimum_required(VERSION 3.10)
project(logiops)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src/logid)

if(NOT EXISTS "${PROJECT_BINARY_DIR}/logid.cfg")
    configure_file("logid.example.cfg" "logid.cfg" COPYONLY)
endif()
