cmake_minimum_required(VERSION 3.10)
project(logid)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../CMake")

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

add_executable(logid
        logid.cpp
        util.cpp
        util.h
        Configuration.cpp
        Configuration.h
        Actions.cpp
        Actions.h
        Device.cpp
        Device.h
        DeviceFinder.cpp
        DeviceFinder.h
        EvdevDevice.cpp
        EvdevDevice.h)
set_target_properties(logid PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

pkg_check_modules(PC_EVDEV libevdev)
pkg_check_modules(SYSTEMD "systemd")

find_path(HIDPP_INCLUDE_DIR hidpp)
find_library(HIDPP_LIBRARY libhidpp.so)

find_path(EVDEV_INCLUDE_DIR libevdev/libevdev.h
          HINTS ${PC_EVDEV_INCLUDE_DIRS} ${PC_EVDEV_INCLUDEDIR})
find_library(EVDEV_LIBRARY
        NAMES evdev libevdev)

include_directories(${HIDPP_INCLUDE_DIR}/hidpp ${EVDEV_INCLUDE_DIR})

target_link_libraries(logid ${CMAKE_THREAD_LIBS_INIT} ${EVDEV_LIBRARY} config++ ${HIDPP_LIBRARY})

install(TARGETS logid DESTINATION bin)

if (SYSTEMD_FOUND AND "${SYSTEMD_SERVICES_INSTALL_DIR}" STREQUAL "")
    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE}
        --variable=systemdsystemunitdir systemd
        OUTPUT_VARIABLE SYSTEMD_SERVICES_INSTALL_DIR)
    string(REGEX REPLACE "[ \t\n]+" "" SYSTEMD_SERVICES_INSTALL_DIR
           "${SYSTEMD_SERVICES_INSTALL_DIR}")
    configure_file(logid.service.cmake ${CMAKE_BINARY_DIR}/logid.service)
    message(STATUS "systemd units will be installed at ${SYSTEMD_SERVICES_INSTALL_DIR}")
    install(FILES ${CMAKE_BINARY_DIR}/logid.service
            DESTINATION ${SYSTEMD_SERVICES_INSTALL_DIR}
            COMPONENT cp)
elseif(NOT SYSTEMD_FOUND AND SYSTEMD_SERVICES_INSTALL_DIR)
    message(FATAL_ERROR "systemd is not found w/ pkg-config but SYSTEMD_SERVICES_INSTALL_DIR is defined.")
endif()
