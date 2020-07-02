cmake_minimum_required(VERSION 3.10)
project(logid)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(DBusCXX REQUIRED)

add_executable(logid
        logid.cpp
        util/log.cpp
        DeviceManager.cpp
        Device.cpp
        Receiver.cpp
        Configuration.cpp
        features/DPI.cpp
        features/SmartShift.cpp
        backend/Error.cpp
        backend/raw/DeviceMonitor.cpp
        backend/raw/RawDevice.cpp
        backend/dj/Receiver.cpp
        backend/dj/ReceiverMonitor.cpp
        backend/dj/Error.cpp
        backend/hidpp/Device.cpp
        backend/hidpp/Report.cpp
        backend/hidpp10/Error.cpp
        backend/hidpp10/Device.cpp
        backend/hidpp20/Device.cpp
        backend/hidpp20/Error.cpp
        backend/hidpp20/Feature.cpp
        backend/hidpp20/EssentialFeature.cpp
        backend/hidpp20/features/Root.cpp
        backend/hidpp20/features/FeatureSet.cpp
        backend/hidpp20/features/DeviceName.cpp
        backend/hidpp20/features/Reset.cpp
        backend/hidpp20/features/AdjustableDPI.cpp
        backend/hidpp20/features/SmartShift.cpp
        backend/hidpp20/features/ReprogControls.cpp
        backend/dj/Report.cpp
        util/mutex_queue.h
        util/thread.cpp
        util/ExceptionHandler.cpp)

set_target_properties(logid PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

pkg_check_modules(PC_EVDEV libevdev REQUIRED)
pkg_check_modules(SYSTEMD "systemd")
pkg_check_modules(LIBCONFIG libconfig REQUIRED)
pkg_check_modules(LIBUDEV libudev REQUIRED)

find_path(EVDEV_INCLUDE_DIR libevdev/libevdev.h
          HINTS ${PC_EVDEV_INCLUDE_DIRS} ${PC_EVDEV_INCLUDEDIR})
find_library(EVDEV_LIBRARY
        NAMES evdev libevdev)

include_directories(${EVDEV_INCLUDE_DIR} ${DBUSCXX_INCLUDE_DIR} ${LIBUDEV_INCLUDE_DIRECTORIES})

target_link_libraries(logid ${CMAKE_THREAD_LIBS_INIT} ${EVDEV_LIBRARY} config++
        ${DBUSCXX_LIBRARIES} ${LIBUDEV_LIBRARIES})

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
