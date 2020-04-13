cmake_minimum_required(VERSION 3.10)
project(logid)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(DBusCXX REQUIRED)

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
        EvdevDevice.h
        IPCServer.cpp
        IPCServer.h)

set_target_properties(logid PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

pkg_check_modules(PC_EVDEV libevdev)
pkg_check_modules(SYSTEMD "systemd")
pkg_check_modules(LIBCONFIG libconfig)

find_path(HIDPP_INCLUDE_DIR hidpp)
find_library(HIDPP_LIBRARY libhidpp.so)

find_path(EVDEV_INCLUDE_DIR libevdev/libevdev.h
          HINTS ${PC_EVDEV_INCLUDE_DIRS} ${PC_EVDEV_INCLUDEDIR})
find_library(EVDEV_LIBRARY
        NAMES evdev libevdev)

if((NOT HIDPP_INCLUDE_DIR) OR (NOT EXISTS ${HIDPP_INCLUDE_DIR}) OR (NOT HIDPP_LIBRARY) OR FORCE_BUILD_HIDPP)
    message("Could not find libhidpp include dir, getting submodule")

    execute_process(COMMAND git submodule update --init -- hidpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	    set(DEFAULT_HID_BACKEND "linux")
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	    set(DEFAULT_HID_BACKEND "windows")
    else()
	    message(WARNING "System is not supported")
    endif()

    set(HID_BACKEND "${DEFAULT_HID_BACKEND}" CACHE STRING "Backend used for accessing HID devices")
    set_property(CACHE HID_BACKEND PROPERTY STRINGS linux windows)
    
    if("${HID_BACKEND}" STREQUAL "linux")
	    pkg_check_modules(LIBUDEV libudev REQUIRED)
    elseif("${HID_BACKEND}" STREQUAL "windows")
    	add_definitions(-DUNICODE -D_UNICODE)
	    add_definitions(-D_WIN32_WINNT=0x0600) # Use vista or later
    else()
	    message(FATAL_ERROR "HID_BACKEND is invalid.")
    endif()

    add_subdirectory(hidpp/src/libhidpp)

    set(HIDPP_INCLUDE_DIR "hidpp/src/libhidpp/")
    set(HIDPP_LIBRARY hidpp)
else()
    set(HIDPP_INCLUDE_DIR ${HIDPP_INCLUDE_DIR}/hidpp)
endif()

include_directories(${HIDPP_INCLUDE_DIR} ${EVDEV_INCLUDE_DIR} ${DBUSCXX_INCLUDE_DIR})

target_link_libraries(logid ${CMAKE_THREAD_LIBS_INIT} ${EVDEV_LIBRARY} config++ ${HIDPP_LIBRARY}
        ${DBUSCXX_LIBRARIES})

configure_file(${CMAKE_SOURCE_DIR}/dbus/logid.xml ${CMAKE_BINARY_DIR}/dbus/logid.xml)

install(TARGETS logid DESTINATION bin)
install(FILES ${CMAKE_BINARY_DIR}/dbus/logid.xml
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/logiops/dbus/logid.xml
        COMPONENT cp)

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
