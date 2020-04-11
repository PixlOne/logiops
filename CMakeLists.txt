cmake_minimum_required(VERSION 3.10)

set(CMAKE_INSTALL_PREFIX /usr)

project(logiops)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -Wextra")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Git)

# Set version number
if(EXISTS ${CMAKE_SOURCE_DIR}/version.txt)
    file(READ version.txt LOGIOPS_VERSION)
    string(REGEX REPLACE "\n$" "" LOGIOPS_VERSION ${LOGIOPS_VERSION})
endif()

if(GIT_FOUND AND EXISTS ${CMAKE_SOURCE_DIR}/.git)
    execute_process(COMMAND ${GIT_EXECUTABLE}
            rev-parse --abbrev-ref HEAD
            OUTPUT_VARIABLE LOGIOPS_GIT_BRANCH)
    string(REGEX REPLACE "\n$" "" LOGIOPS_GIT_BRANCH ${LOGIOPS_GIT_BRANCH})
    if(LOGIOPS_GIT_BRANCH MATCHES "^tags/?")
        STRING(REGEX REPLACE "^tags/" ""
                LOGIOPS_VERSION ${LOGIOPS_GIT_BRANCH})
    else()
        execute_process(COMMAND ${GIT_EXECUTABLE}
                rev-parse --short HEAD
                OUTPUT_VARIABLE LOGIOPS_COMMIT_HASH)
        string(REGEX REPLACE "\n$" "" LOGIOPS_COMMIT_HASH ${LOGIOPS_COMMIT_HASH})
        if(LOGIOPS_VERSION)
            string(APPEND LOGIOPS_VERSION -${LOGIOPS_COMMIT_HASH})
        else()
            set(LOGIOPS_VERSION git-${LOGIOPS_COMMIT_HASH})
        endif()
    endif()
endif()

if(LOGIOPS_VERSION)
    message("LogiOps Version Number: ${LOGIOPS_VERSION}")
else()
    set(LOGIOPS_VERSION "null")
endif()

add_definitions( -DLOGIOPS_VERSION="${LOGIOPS_VERSION}")

add_subdirectory(src/logid)
add_subdirectory(src/liblogiops)
add_subdirectory(src/logictl)