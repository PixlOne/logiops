cmake_minimum_required(VERSION 3.12)

set(CMAKE_INSTALL_PREFIX /usr)

project(logiops)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_NONE "${CMAKE_CXX_FLAGS_NONE} -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -Wextra")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_USER_BUS "Uses user bus" OFF)

find_package(Git)

# Set version number and update submodules
if(EXISTS ${PROJECT_SOURCE_DIR}/.git AND GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags
         OUTPUT_VARIABLE LOGIOPS_VERSION
         RESULT_VARIABLE LOGIOPS_VERSION_RET
         WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
         ERROR_QUIET)
    if(NOT LOGIOPS_VERSION_RET EQUAL "0")
        execute_process(COMMAND ${GIT_EXECUTABLE}
            rev-parse --short HEAD
            OUTPUT_VARIABLE LOGIOPS_COMMIT_HASH)
        set(LOGIOPS_VERSION git-${LOGIOPS_COMMIT_HASH})
    endif()

    execute_process(COMMAND ${GIT_EXECUTABLE}
            submodule update --init --recursive
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

    string(REGEX REPLACE "\n$" "" LOGIOPS_VERSION ${LOGIOPS_VERSION})
elseif(EXISTS ${PROJECT_SOURCE_DIR}/version.txt)
    file(READ version.txt LOGIOPS_VERSION)
    string(REGEX REPLACE "\n$" "" LOGIOPS_VERSION ${LOGIOPS_VERSION})

endif()

IF(NOT EXISTS ${PROJECT_SOURCE_DIR}/src/ipcgull)
    message(FATAL_ERROR "Missing ipcgull submodule")
endif()

if(NOT LOGIOPS_VERSION)
    set(LOGIOPS_VERSION "null")
endif()
message("LogiOps Version Number: ${LOGIOPS_VERSION}")

add_definitions( -DLOGIOPS_VERSION="${LOGIOPS_VERSION}")

if(USE_USER_BUS)
    add_definitions(-DUSE_USER_BUS)
endif()

add_subdirectory(src/ipcgull)
add_subdirectory(src/logid)
