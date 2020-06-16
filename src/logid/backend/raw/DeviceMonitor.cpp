#include "DeviceMonitor.h"

#include <thread>
#include <system_error>

extern "C"
{
#include <unistd.h>
#include <libudev.h>
}

using namespace logid::backend::raw;

DeviceMonitor::DeviceMonitor()
{
    if(-1 == pipe(monitor_pipe))
        throw std::system_error(errno, std::system_category(), "pipe creation failed");

    udev_context = udev_new();
    if(!udev_context)
        throw std::runtime_error("udev_new failed");
}

DeviceMonitor::~DeviceMonitor()
{
    bool is_running = running.try_lock();
    if(is_running)
        running.unlock();
    else
        this->stop();

    udev_unref(udev_context);

    for(int i : monitor_pipe)
        close(i);
}

void DeviceMonitor::run()
{
    int ret;
    const std::lock_guard<std::mutex> run_lock(running);

    struct udev_monitor* monitor = udev_monitor_new_from_netlink(udev_context, "udev");
    if(!monitor)
        throw std::runtime_error("udev_monitor_new_from_netlink failed");

    ret = udev_monitor_filter_add_match_subsystem_devtype(monitor, "hidraw", nullptr);
    if (0 != ret)
        throw std::system_error (-ret, std::system_category (),
                "udev_monitor_filter_add_match_subsystem_devtype");

    ret = udev_monitor_enable_receiving(monitor);
    if(0 != ret)
        throw std::system_error(-ret, std::system_category(),
                "udev_moniotr_enable_receiving");

    this->enumerate();

    int fd = udev_monitor_get_fd(monitor);
    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(monitor_pipe[0], &fds);
        FD_SET(fd, &fds);
        if (-1 == select (std::max (monitor_pipe[0], fd)+1, &fds, nullptr, nullptr, nullptr)) {
            if (errno == EINTR)
                continue;
            throw std::system_error (errno, std::system_category(), "udev_monitor select");
        }
        if (FD_ISSET(fd, &fds)) {
            struct udev_device *device = udev_monitor_receive_device(monitor);
            std::string action = udev_device_get_action(device);
            std::string devnode = udev_device_get_devnode(device);
            if (action == "add")
                std::thread([this](const std::string name) {
                    this->addDevice(name);
                }, devnode).detach();
            else if (action == "remove")
                std::thread([this](const std::string name) {
                    this->removeDevice(name);
                }, devnode).detach();
            udev_device_unref (device);
        }
        if (FD_ISSET(monitor_pipe[0], &fds)) {
            char c;
            if (-1 == read(monitor_pipe[0], &c, sizeof (char)))
                throw std::system_error (errno, std::system_category (),
                                         "read pipe");
            break;
        }
    }
}

void DeviceMonitor::stop()
{

}

void DeviceMonitor::enumerate()
{
    int ret;
    struct udev_enumerate* udev_enum = udev_enumerate_new(udev_context);
    ret = udev_enumerate_add_match_subsystem(udev_enum, "hidraw");
    if(0 != ret)
        throw std::system_error(-ret, std::system_category(),
                "udev_enumerate_add_match_subsystem");

    ret = udev_enumerate_scan_devices(udev_enum);
    if(0 != ret)
        throw std::system_error(-ret, std::system_category(),
                                "udev_enumerate_scan_devices");

    struct udev_list_entry* udev_enum_entry;
    udev_list_entry_foreach(udev_enum_entry,
            udev_enumerate_get_list_entry(udev_enum))
    {
        const char* name = udev_list_entry_get_name(udev_enum_entry);

        struct udev_device* device = udev_device_new_from_syspath(udev_context,
                name);
        if(!device)
            throw std::runtime_error("udev_device_new_from_syspath failed");

        std::string devnode = udev_device_get_devnode(device);
        udev_device_unref(device);

        std::thread([this](const std::string name) {
            this->addDevice(name);
        }, devnode).detach();
    }

    udev_enumerate_unref(udev_enum);
}